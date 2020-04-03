#include "archive.h"

#include <cstring>
#include <fstream>
#include <locale>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <codecvt.h>
#include <logger.h>

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
    std::string path;
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
    explicit XP3Archive(const my::fs::path &path) : _archive_path(path) {
        std::ifstream xp3_archive;
        xp3_archive.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        xp3_archive.open(this->_archive_path, std::ios::binary);

        XP3ArchiveHeader header;

        xp3_archive.read((char *)&header, sizeof(XP3ArchiveHeader));
        if (std::memcmp(header.mark1, XP3Mark1, sizeof(header.mark1)) ||
            std::memcmp(header.mark2, XP3Mark2, sizeof(header.mark2))) {
            throw std::runtime_error(
                (boost::format("xp3 archive: %1 format error") % path).str());
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
        utf16_codecvt code_cvt;

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
                file_info->path.resize(info.len);
                std::u16string s(info.len, 0);
                read(s.data(), info.len * 2);
                file_info->path = code_cvt.to_bytes(s);

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
                    (boost::format("xp3 archive: unknown chunk %1") %
                     std::string(chunk.type, 4))
                        .str());
            }
        }
        GLOG_D("read size %d, index size %d", read_size, index_size);
    }

    ~XP3Archive() override {}

    bool exists(const my::fs::path &path) override {
        return this->_xp3_index.find(boost::algorithm::to_lower_copy(
                   path.string())) != this->_xp3_index.end();
    }

    class XP3Stream : public my::Archive::Stream {
      public:
        XP3Stream(const my::fs::path &archive_path,
                  const std::shared_ptr<XP3ArchiveFileInfo> &file_info)
            : _file_info(file_info) {
            this->_archive.exceptions(std::ifstream::failbit |
                                      std::ifstream::badbit);
            this->_archive.open(archive_path, std::ios::binary);
        }

        std::string read_all() override {
            std::string buf(this->_file_info->org_size, 0);

            size_t size = 0;
            for (auto &segm : this->_file_info->segms) {
                this->_archive.seekg(segm.start);

                boost::iostreams::filtering_istream ss_decomp;
                ss_decomp.exceptions(std::ifstream::failbit);
                ss_decomp.push(boost::iostreams::zlib_decompressor());
                ss_decomp.push(this->_archive);

                ss_decomp.read(buf.data() + size, segm.org_size);
                size += segm.org_size;
            }
            return buf;
        }

      private:
        std::ifstream _archive;
        std::shared_ptr<XP3ArchiveFileInfo> _file_info;
    };

    std::shared_ptr<my::Archive::Stream>
    open(const my::fs::path &path) override {
        if (!path.is_relative()) {
            throw std::runtime_error(
                (boost::format("xp3 archive: path format error %1") % path)
                    .str());
        }

        auto it = this->_xp3_index.find(
            boost::algorithm::to_lower_copy(path.string()));
        if (it == this->_xp3_index.end()) {
            throw std::runtime_error(
                (boost::format("xp3 archive: %1 is not exist") % path).str());
        }

        return std::make_shared<XP3Stream>(this->_archive_path, it->second);
    }

  private:
    my::fs::path _archive_path;
    std::unordered_map<std::string, std::shared_ptr<XP3ArchiveFileInfo>>
        _xp3_index;
};

} // namespace

namespace my {
std::shared_ptr<Archive> Archive::make_xp3(const fs::path &path) {
    return std::make_shared<XP3Archive>(path);
}
} // namespace my
