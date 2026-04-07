#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class NavigationState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(QStringList availablePages READ availablePages CONSTANT)

public:
    explicit NavigationState(QObject *parent = nullptr);

    QString currentPage() const;
    QStringList availablePages() const;

    Q_INVOKABLE void setCurrentPage(const QString &page);

signals:
    void currentPageChanged();

private:
    QString m_currentPage;
    QStringList m_availablePages;
};