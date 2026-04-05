#pragma once

#include <QtGlobal>

namespace desktop {

enum class PageId {
    Dashboard = 0,
    ScanViewer,
    Pipelines,
    Analytics,
};

}  // namespace desktop

inline size_t qHash(desktop::PageId page_id, size_t seed = 0) {
    return qHash(static_cast<int>(page_id), seed);
}
