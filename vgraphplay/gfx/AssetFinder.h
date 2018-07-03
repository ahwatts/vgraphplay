// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAHPLAY_VGRAPHPLAY_GFX_ASSET_FINDER_H_
#define _VGRAHPLAY_VGRAPHPLAY_GFX_ASSET_FINDER_H_

#include <boost/filesystem.hpp>

namespace vgraphplay {
    namespace gfx {
        class AssetFinder {
        public:
            template <class Source>
            AssetFinder(const Source &bin_dir)
                : m_base_path{bin_dir},
                  m_shaders_path{"."}
            {
                m_shaders_path /= "..";
                m_shaders_path /= "shaders";
            }

            template <class Source>
            boost::filesystem::path findAsset(const Source &relative_path) const {
                boost::filesystem::path asset_path(m_base_path);
                asset_path /= relative_path;
                if (exists(asset_path)) {
                    return canonical(asset_path);
                } else {
                    return asset_path;
                }
            }

            template <class Source>
            boost::filesystem::path findShader(const Source &filename) const {
                boost::filesystem::path asset_path(m_shaders_path);
                asset_path /= filename;
                return findAsset(asset_path);
            }

        protected:
            boost::filesystem::path m_base_path;
            boost::filesystem::path m_shaders_path;
        };
    }
}

#endif

