/*
 * Xenia Edge - Xbox 360 Emulator Libretro Core
 * Shortcuts for Xbox Live Arcade and Games on Demand packages (see
 * libretro_xbla.h). The way Xendroid+ finds them: a package (CON, LIVE or
 * PIRS header) whose content type is a game.
 */

#include "libretro_xbla.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <set>
#include <vector>
#include <string>
#include <system_error>

#include "xenia/base/filesystem.h"
#include "xenia/vfs/stfs_metadata.h"

namespace fs = std::filesystem;

namespace xbla {
namespace {

// Content types that are a game to start, as Xendroid+'s library has them
bool is_game_type(uint32_t type) {
  switch (type) {
    case 0x000D0000:  // Arcade title
    case 0x00007000:  // Games on Demand
    case 0x00004000:  // Installed game
    case 0x00080000:  // Game demo
    case 0x02000000:  // Community (Indie) game
      return true;
    default:
      return false;
  }
}

// A package header: the first four bytes are CON, LIVE or PIRS. Read before
// the full metadata, so disc images and the packages' own data files are not
// opened as packages.
bool has_package_magic(const fs::path &path) {
  std::ifstream in(path, std::ios::binary);
  char magic[4] = {};
  if (!in.read(magic, sizeof(magic))) return false;
  return std::memcmp(magic, "CON ", 4) == 0 ||
         std::memcmp(magic, "LIVE", 4) == 0 ||
         std::memcmp(magic, "PIRS", 4) == 0;
}

// A name RetroArch, Android and Windows all take as a file name
std::string file_name_of(const std::string &title, uint32_t title_id) {
  std::string name;
  for (unsigned char c : title) {
    if (c < 0x20 || std::strchr("\\/:*?\"<>|", c)) continue;
    name += static_cast<char>(c);
  }
  while (!name.empty() && (name.back() == ' ' || name.back() == '.'))
    name.pop_back();
  size_t start = name.find_first_not_of(' ');
  name = start == std::string::npos ? std::string() : name.substr(start);
  char id[16];
  snprintf(id, sizeof(id), "%08X", title_id);
  if (name.empty()) return std::string(id) + ".m3u";
  return name + " [" + id + "].m3u";
}

// The line a shortcut holds: a package in the folder relative to it, so the
// folder can be moved or read through another mount point; one in another
// folder (an SD card) by its full path
std::string line_for(const fs::path &dir, const fs::path &package) {
  std::error_code ec;
  fs::path rel = fs::relative(package, dir, ec);
  const bool inside = !ec && !rel.empty() && *rel.begin() != "..";
  return xe::path_to_utf8(inside ? rel : package);
}

// xbla_folders.txt: one more folder a line, # starts a comment
std::vector<fs::path> extra_folders(const fs::path &list) {
  std::vector<fs::path> folders;
  std::ifstream in(list);
  std::string line;
  while (std::getline(in, line)) {
    if (auto hash = line.find('#'); hash != std::string::npos) line.erase(hash);
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ' ||
                             line.back() == '\t'))
      line.pop_back();
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    if (line.size() >= 3 && std::memcmp(line.data(), "\xEF\xBB\xBF", 3) == 0)
      start = std::max<size_t>(start, 3);
    folders.push_back(xe::to_path(line.substr(start)));
  }
  return folders;
}

constexpr char kFoldersTemplate[] =
    "# More folders with Xbox Live Arcade and Games on Demand games, one a\n"
    "# line, for example a folder on an SD card:\n"
    "#   /storage/1234-ABCD/XBLA\n"
    "# (Android shows the card's ID in its file manager; RetroArch can read\n"
    "# it once allowed to manage all files.) The core looks in them, and in\n"
    "# the xbla folder next to this file, each time RetroArch loads it, and\n"
    "# puts the .m3u for each game in that xbla folder.\n";

constexpr char kHeader[] =
    "# Made by the Xenia Edge core for an Xbox Live Arcade or Games on Demand\n"
    "# package; it is rewritten or deleted as the package changes.\n";

bool is_ours(const fs::path &m3u) {
  std::ifstream in(m3u);
  std::string first;
  std::getline(in, first);
  return first.rfind("# Made by the Xenia Edge core", 0) == 0;
}

}  // namespace

fs::path folder(const fs::path &root) { return root / "xbla"; }

fs::path folders_file(const fs::path &root) {
  return root / "xbla_folders.txt";
}

int make_shortcuts(const fs::path &root, const Log &log) {
  const fs::path dir = folder(root);
  std::error_code ec;
  fs::create_directories(dir, ec);
  if (!fs::is_directory(dir, ec)) return 0;

  // The list of more folders is the user's: made with only comments when
  // missing, never overwritten
  const fs::path list = folders_file(root);
  if (!fs::exists(list, ec)) {
    std::ofstream out(list, std::ios::binary);
    out << kFoldersTemplate;
  }
  std::vector<fs::path> scan{dir};
  bool all_reachable = true;
  for (const fs::path &extra : extra_folders(list)) {
    if (fs::is_directory(extra, ec)) {
      scan.push_back(extra);
    } else {
      // An SD card not in right now: its games' shortcuts are kept
      all_reachable = false;
      log("XBLA: cannot open " + xe::path_to_utf8(extra) +
          " (listed in xbla_folders.txt)");
    }
  }

  constexpr int kMaxDepth = 8;
  constexpr int kMaxEntries = 20000;
  int games = 0;
  std::set<fs::path> written;

  for (const fs::path &top : scan) {
    int entries = 0;
    fs::recursive_directory_iterator it(
        top, fs::directory_options::skip_permission_denied, ec);
    for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
      if (++entries > kMaxEntries) {
        log("XBLA: stopped after " + std::to_string(kMaxEntries) +
            " entries in " + xe::path_to_utf8(top));
        break;
      }
      if (it.depth() >= kMaxDepth) it.disable_recursion_pending();
      const fs::directory_entry &entry = *it;
      std::error_code fec;
      if (!entry.is_regular_file(fec)) continue;
      const fs::path path = entry.path();
      if (entry.file_size(fec) < 0x1000 || !has_package_magic(path)) continue;

      auto meta = xe::vfs::ExtractStfsMetadata(path);
      if (!meta || !meta->title_id || !is_game_type(meta->content_type))
        continue;
      const std::string &title =
          !meta->title_name.empty() ? meta->title_name : meta->display_name;
      const fs::path m3u =
          dir / xe::to_path(file_name_of(title, meta->title_id));
      if (written.count(m3u)) continue;  // a second package of the same name
      written.insert(m3u);
      ++games;

      const std::string body =
          std::string(kHeader) + line_for(dir, path) + "\n";
      std::string existing;
      {
        std::ifstream in(m3u, std::ios::binary);
        existing.assign(std::istreambuf_iterator<char>(in), {});
      }
      if (existing == body) continue;
      if (!existing.empty() && !is_ours(m3u)) continue;  // the user's own file
      std::ofstream out(m3u, std::ios::binary | std::ios::trunc);
      out << body;
      log("XBLA: " + xe::path_to_utf8(m3u.filename()) + " -> " +
          xe::path_to_utf8(path));
    }
    ec.clear();
  }

  // Shortcuts of ours whose package is gone - not while a listed folder is
  // out of reach, whose games may be among them
  if (all_reachable) {
    for (const auto &entry : fs::directory_iterator(dir, ec)) {
      const fs::path &p = entry.path();
      if (p.extension() != ".m3u" || written.count(p) || !is_ours(p)) continue;
      fs::remove(p, ec);
      log("XBLA: removed " + xe::path_to_utf8(p.filename()) +
          ", its package is gone");
    }
  }
  return games;
}

}  // namespace xbla
