/*
 * Xenia Edge - Xbox 360 Emulator Libretro Core
 * Xbox Live Arcade and Games on Demand packages as content RetroArch can
 * open: the packages have no file extension, which RetroArch's file browser
 * hides, so the core lists them as .m3u files it can see.
 */

#ifndef LIBRETRO_XBLA_H
#define LIBRETRO_XBLA_H

#include <filesystem>
#include <functional>
#include <string>

namespace xbla {

// The folder the packages go in: <system>/Xenia-Edge/xbla. Any layout under
// it works, the console's own (<Title ID>/000D0000/<package>) included.
std::filesystem::path folder(const std::filesystem::path &root);

// Walks folder(root) for game packages (Arcade titles, Games on Demand,
// installed games, demos) and writes "<game name> [<Title ID>].m3u" for each
// in the folder itself, pointing at the package; a shortcut whose package is
// gone is deleted. Returns how many games it found. Bounded: depth 8, 20000
// entries. Each shortcut written or removed is told to log.
using Log = std::function<void(const std::string &)>;
int make_shortcuts(const std::filesystem::path &root, const Log &log);

}  // namespace xbla

#endif  // LIBRETRO_XBLA_H
