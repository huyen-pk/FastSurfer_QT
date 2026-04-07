#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include "app/navigation_state.h"

class ApplicationController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)
    Q_PROPERTY(QString productName READ productName CONSTANT)
    Q_PROPERTY(QString projectName READ projectName CONSTANT)
    Q_PROPERTY(QString projectContext READ projectContext CONSTANT)
    Q_PROPERTY(NavigationState *navigation READ navigation CONSTANT)
    Q_PROPERTY(QVariantList dashboardMetrics READ dashboardMetrics CONSTANT)
    Q_PROPERTY(QVariantList studies READ studies CONSTANT)
    Q_PROPERTY(QVariantList operationalPanels READ operationalPanels CONSTANT)
    Q_PROPERTY(QVariantList viewerPanels READ viewerPanels CONSTANT)
    Q_PROPERTY(QVariantList scanMetadata READ scanMetadata CONSTANT)
    Q_PROPERTY(QVariantList studyNotes READ studyNotes CONSTANT)
    Q_PROPERTY(QVariantList pipelineSteps READ pipelineSteps CONSTANT)
    Q_PROPERTY(QVariantList analyticsBars READ analyticsBars CONSTANT)
    Q_PROPERTY(QVariantList analyticsCards READ analyticsCards CONSTANT)
    Q_PROPERTY(QVariantList analyticsInsights READ analyticsInsights CONSTANT)

public:
    explicit ApplicationController(QObject *parent = nullptr);

    QString applicationName() const;
    QString productName() const;
    QString projectName() const;
    QString projectContext() const;
    NavigationState *navigation();

    QVariantList dashboardMetrics() const;
    QVariantList studies() const;
    QVariantList operationalPanels() const;
    QVariantList viewerPanels() const;
    QVariantList scanMetadata() const;
    QVariantList studyNotes() const;
    QVariantList pipelineSteps() const;
    QVariantList analyticsBars() const;
    QVariantList analyticsCards() const;
    QVariantList analyticsInsights() const;

private:
    NavigationState m_navigation;
    QVariantList m_dashboardMetrics;
    QVariantList m_studies;
    QVariantList m_operationalPanels;
    QVariantList m_viewerPanels;
    QVariantList m_scanMetadata;
    QVariantList m_studyNotes;
    QVariantList m_pipelineSteps;
    QVariantList m_analyticsBars;
    QVariantList m_analyticsCards;
    QVariantList m_analyticsInsights;
};