#include "archive.h"

#include <cstring>
#include <locale>

#include <boost/format.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>

#include <my_gui.hpp>
#include <storage/resource.hpp>

namespace {

struct XP3ArchiveHeader {
    uint8_t mark1[8];
    uint8_t mark2[3];
    uint64_t index_ofs;
} __attribute__((packed));

enum EncodeMethod { RAW = 0, ZLIB = 1 };

struct XP3ArchiveIndex {
    uint8_t encode_method : 3;
    uint8_t reserved : 5;

    union {
        uint64_t compressed_size;
        uint64_t index_size;
    };
    uint64_t uncompress_size[0];
} __attribute__((packed));

struct XP3ArchiveChunk {
    char type[4];
    uint64_t size;
} __attribute__((packed));

struct XP3ArchiveChunkInfo {
    uint32_t flag;
    uint64_t org_size;
    uint64_t arc_size;
    uint16_t len;
    uint16_t name[0];
} __attribute__((packed));

struct XP3ArchiveChunkSegm {
    uint32_t encode_method : 3;
    uint32_t reserved : 29;
    uint64_t start;
    uint64_t org_size;
    uint64_t arc_size;
} __attribute__((packed));

struct XP3ArchiveChunkAldr {
    uint32_t hash;
} __attribute__((packed));

struct XP3ArchiveFileInfo {
    my::fs::path path;
    uint32_t flag;
    uint64_t org_size;
    uint64_t arc_size;
    uint32_t hash;
    std::vector<XP3ArchiveChunkSegm> segms;
};

static uint8_t XP3Mark1[] = {
    0x58 /*'X'*/,  0x50 /*'P'*/, 0x33 /*'3'*/,  0x0d /*'\r'*/,
    0x0a /*'\n'*/, 0x20 /*' '*/, 0x0a /*'\n'*/, 0x1a /*EOF*/
};
static uint8_t XP3Mark2[] = {0x8b, 0x67, 0x01};

class XP3Archive : public my::Archive {
  public:
    explicit XP3Archive(const my::fs::path &path)
        : _archive_path(my::fs::absolute(path)) {
        std::ifstream xp3_archive;
        xp3_archive.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        xp3_archive.open(this->_archive_path, std::ios::binary);

        XP3ArchiveHeader header;

        xp3_archive.read((char *)&header, sizeof(XP3ArchiveHeader));
        if (std::memcmp(header.mark1, XP3Mark1, sizeof(header.mark1)) ||
            std::memcmp(header.mark2, XP3Mark2, sizeof(header.mark2))) {
            throw std::runtime_error(
                (boost::format("xp3 archive: %1% format error") % path).str());
        }

        xp3_archive.seekg(header.index_ofs);

        XP3ArchiveIndex index;
        xp3_archive.read((char *)&index, sizeof(XP3ArchiveIndex));
        uint64_t index_size = index.index_size;
        if (index.encode_method == EncodeMethod::ZLIB) {
            xp3_archive.read((char *)index.uncompress_size, sizeof(uint64_t));
            index_size = index.uncompress_size[0];
        }

        boost::iostreams::filtering_istream ss_decomp;
        ss_decomp.exceptions(std::ifstream::failbit);
        ss_decomp.push(boost::iostreams::zlib_decompressor());
        ss_decomp.push(xp3_archive);
        static uint8_t chunk_file[] = {0x46 /*'F'*/, 0x69 /*'i'*/, 0x6c /*'l'*/,
                                       0x65 /*'e'*/};
        static uint8_t chunk_info[] = {0x69 /*'i'*/, 0x6e /*'n'*/, 0x66 /*'f'*/,
                                       0x6f /*'o'*/};
        static uint8_t chunk_segm[] = {0x73 /*'s'*/, 0x65 /*'e'*/, 0x67 /*'g'*/,
                                       0x6d /*'m'*/};
        static uint8_t chunk_adlr[] = {0x61 /*'a'*/, 0x64 /*'d'*/, 0x6c /*'l'*/,
                                       0x72 /*'r'*/};

        std::shared_ptr<XP3ArchiveFileInfo> file_info = nullptr;

        uint64_t read_size = 0;
        auto read = [&ss_decomp, &read_size](void *data, size_t size) {
            ss_decomp.read((char *)data, size);
            read_size += size;
        };

        while (read_size < index_size) {
            XP3ArchiveChunk chunk{};
            read(&chunk, sizeof(XP3ArchiveChunk));

            if (!std::memcmp(chunk.type, chunk_file, 4)) {
                assert(!file_info);
                file_info = std::make_shared<XP3ArchiveFileInfo>();
            } else if (!std::memcmp(chunk.type, chunk_info, 4)) {
                assert(file_info);
                XP3ArchiveChunkInfo info{};
                read(&info, sizeof(XP3ArchiveChunkInfo));

                file_info->org_size = info.org_size;
                file_info->arc_size = info.arc_size;
                file_info->flag = info.flag;
                std::u16string path(info.len, 0);
                read(path.data(), info.len * 2);
                auto utf8_path = codecvt::utf_to_utf<char>(path);
                std::replace(utf8_path.begin(), utf8_path.end(), '\\', '/');
                file_info->path = utf8_path;

            } else if (!std::memcmp(chunk.type, chunk_segm, 4)) {
                assert(file_info);
                file_info->segms.resize(chunk.size /
                                        sizeof(XP3ArchiveChunkSegm));
                read(file_info->segms.data(), chunk.size);
            } else if (!std::memcmp(chunk.type, chunk_adlr, 4)) {
                assert(file_info);
                XP3ArchiveChunkAldr aldr{};
                read(&aldr, sizeof(XP3ArchiveChunkAldr));
                file_info->hash = aldr.hash;
                this->_xp3_index.insert({file_info->path, file_info});
                file_info.reset();
            } else {
                throw std::runtime_error(
                    (boost::format("xp3 archive: unknown chunk %1%") %
                     std::string(chunk.type, 4))
                        .str());
            }
        }
        GLOG_D("read size %d, index size %d", read_size, index_size);
    }

    ~XP3Archive() override {}

    bool exists(const my::fs::path &path) override {
        return this->_xp3_index.find(path) != this->_xp3_index.end();
    }

    std::vector<std::string> list_files() override {
        std::vector<std::string> v;
        std::transform(
            this->_xp3_index.begin(), this->_xp3_index.end(),
            std::back_inserter(v),
            [](const xp3_index_map::value_type &it) { return it.first; });
        return v;
    }

    class XP3Source : public boost::iostreams::source {
      public:
        XP3Source(const my::fs::path &archive_path,
                  const std::shared_ptr<XP3ArchiveFileInfo> &file_info)
            : _archive_path(archive_path), _file_info(file_info) {
            this->_archive = my::make_ifstream(archive_path);

            this->_current_segm = this->_file_info->segms.begin();
            init_current_segm();
        }

        void init_current_segm() {
            if (this->_current_segm == this->_file_info->segms.end()) {
                return;
            }
            this->_archive->seekg(this->_current_segm->start);
            this->_stream =
                std::make_shared<boost::iostreams::filtering_istream>();
            this->_stream->exceptions(std::ifstream::failbit);
            if (this->_current_segm->encode_method == EncodeMethod::ZLIB) {
                this->_stream->push(boost::iostreams::zlib_decompressor());
            }
            this->_stream->push(*this->_archive);
        }

        std::streamsize read(char *s, std::streamsize n) {
            if (this->_current_segm == this->_file_info->segms.end()) {
                return -1; // EOF
            }

            std::streamsize readed = 0;

            while (true) {

                if (this->_segm_readed + n >
                    static_cast<std::streamsize>(
                        this->_current_segm->org_size)) {
                    auto size =
                        this->_current_segm->org_size - this->_segm_readed;
                    this->_stream->read(s, size);
                    readed += size;
                    s += size;
                    n -= size;
                } else {
                    this->_segm_readed += n;
                    readed += n;
                    this->_stream->read(s, n);
                    break;
                }

                this->_segm_readed = 0;
                ++this->_current_segm;
                init_current_segm();
                if (this->_current_segm == this->_file_info->segms.end()) {
                    break;
                }
            }
            return readed;
        }

      private:
        my::fs::path _archive_path;
        std::shared_ptr<std::ifstream> _archive;
        std::shared_ptr<XP3ArchiveFileInfo> _file_info;
        std::vector<XP3ArchiveChunkSegm>::iterator _current_segm;
        std::streamsize _segm_readed{0};
        std::shared_ptr<boost::iostreams::filtering_istream> _stream;
    };

    my::ArchiveFile extract(const my::fs::path &path) override {
        if (!path.is_relative()) {
            throw std::runtime_error(
                (boost::format("xp3 archive: path format error %1%") % path)
                    .str());
        }

        auto it = this->_xp3_index.find(path.string());
        if (it == this->_xp3_index.end()) {
            throw std::runtime_error(
                (boost::format("xp3 archive: %1% is not exist") % path).str());
        }
        auto info = it->second;

        return my::ArchiveFile{
            info->path, info->arc_size, info->org_size,
            std::make_unique<boost::iostreams::stream<XP3Source>>(
                this->_archive_path, it->second)};
    }

  private:
    my::fs::path _archive_path;
    using xp3_index_map =
        std::map<my::fs::path, std::shared_ptr<XP3ArchiveFileInfo>>;
    xp3_index_map _xp3_index;
};

} // namespace

namespace my {
std::shared_ptr<Archive> open_xp3(const fs::path &path) {
    return std::make_shared<XP3Archive>(path);
}
} // namespace my
