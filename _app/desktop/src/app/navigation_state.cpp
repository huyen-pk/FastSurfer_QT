#include "app/navigation_state.h"

NavigationState::NavigationState(QObject *parent)
    : QObject(parent)
    , m_currentPage(QStringLiteral("dashboard"))
    , m_availablePages({
          QStringLiteral("dashboard"),
          QStringLiteral("scan_viewer"),
          QStringLiteral("pipelines"),
          QStringLiteral("analytics")
      })
{
}

QString NavigationState::currentPage() const
{
    return m_currentPage;
}

QStringList NavigationState::availablePages() const
{
    return m_availablePages;
}

void NavigationState::setCurrentPage(const QString &page)
{
    if (!m_availablePages.contains(page) || m_currentPage == page) {
        return;
    }

    m_currentPage = page;
    emit currentPageChanged();
}