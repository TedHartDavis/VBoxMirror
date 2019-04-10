/* $Id$ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageBasic1 class implementation.
 */

/*
 * Copyright (C) 2009-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QStackedLayout>
#include <QTableWidget>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIComboBox.h"
#include "QIRichTextLabel.h"
#include "QIToolButton.h"
#include "VBoxGlobal.h"
#include "UIEmptyFilePathSelector.h"
#include "UIIconPool.h"
#include "UIMessageCenter.h"
#include "UIVirtualBoxManager.h"
#include "UIWizardImportApp.h"
#include "UIWizardImportAppPageBasic1.h"
#include "UIWizardImportAppPageBasic2.h"


/*********************************************************************************************************************************
*   Class UIWizardImportAppPage1 implementation.                                                                                 *
*********************************************************************************************************************************/

UIWizardImportAppPage1::UIWizardImportAppPage1(bool fImportFromOCIByDefault)
    : m_fImportFromOCIByDefault(fImportFromOCIByDefault)
    , m_pSourceLayout(0)
    , m_pSourceLabel(0)
    , m_pSourceComboBox(0)
    , m_pStackedLayout(0)
    , m_pFileSelector(0)
    , m_pCloudContainerLayout(0)
    , m_pAccountLabel(0)
    , m_pAccountComboBox(0)
    , m_pAccountToolButton(0)
    , m_pAccountPropertyTable(0)
{
}

void UIWizardImportAppPage1::populateSources()
{
    AssertReturnVoid(m_pSourceComboBox->count() == 0);

    /* Compose hardcoded sources list: */
    QStringList sources;
    sources << "local";
    /* Add that list to combo: */
    foreach (const QString &strShortName, sources)
    {
        /* Compose empty item, fill it's data: */
        m_pSourceComboBox->addItem(QString());
        m_pSourceComboBox->setItemData(m_pSourceComboBox->count() - 1, strShortName, SourceData_ShortName);
    }

    /* Initialize Cloud Provider Manager: */
    bool fOCIPresent = false;
    CVirtualBox comVBox = vboxGlobal().virtualBox();
    m_comCloudProviderManager = comVBox.GetCloudProviderManager();
    /* Show error message if necessary: */
    if (!comVBox.isOk())
        msgCenter().cannotAcquireCloudProviderManager(comVBox);
    else
    {
        /* Acquire existing providers: */
        const QVector<CCloudProvider> providers = m_comCloudProviderManager.GetProviders();
        /* Show error message if necessary: */
        if (!m_comCloudProviderManager.isOk())
            msgCenter().cannotAcquireCloudProviderManagerParameter(m_comCloudProviderManager);
        else
        {
            /* Iterate through existing providers: */
            foreach (const CCloudProvider &comProvider, providers)
            {
                /* Skip if we have nothing to populate (file missing?): */
                if (comProvider.isNull())
                    continue;

                /* Compose empty item, fill it's data: */
                m_pSourceComboBox->addItem(QString());
                m_pSourceComboBox->setItemData(m_pSourceComboBox->count() - 1, comProvider.GetId(),        SourceData_ID);
                m_pSourceComboBox->setItemData(m_pSourceComboBox->count() - 1, comProvider.GetName(),      SourceData_Name);
                m_pSourceComboBox->setItemData(m_pSourceComboBox->count() - 1, comProvider.GetShortName(), SourceData_ShortName);
                m_pSourceComboBox->setItemData(m_pSourceComboBox->count() - 1, true,                       SourceData_IsItCloudFormat);
                if (m_pSourceComboBox->itemData(m_pSourceComboBox->count() - 1, SourceData_ShortName).toString() == "OCI")
                    fOCIPresent = true;
            }
        }
    }

    /* Set default: */
    if (m_fImportFromOCIByDefault && fOCIPresent)
        setSource("OCI");
    else
        setSource("local");
}

void UIWizardImportAppPage1::populateAccounts()
{
    /* Block signals while updating: */
    m_pAccountComboBox->blockSignals(true);

    /* Remember current item data to be able to restore it: */
    QString strOldData;
    if (m_pAccountComboBox->currentIndex() != -1)
        strOldData = m_pAccountComboBox->itemData(m_pAccountComboBox->currentIndex(), AccountData_ProfileName).toString();

    /* Clear combo initially: */
    m_pAccountComboBox->clear();
    /* Clear Cloud Provider: */
    m_comCloudProvider = CCloudProvider();

    /* If provider chosen: */
    if (!sourceId().isNull())
    {
        /* (Re)initialize Cloud Provider: */
        m_comCloudProvider = m_comCloudProviderManager.GetProviderById(sourceId());
        /* Show error message if necessary: */
        if (!m_comCloudProviderManager.isOk())
            msgCenter().cannotFindCloudProvider(m_comCloudProviderManager, sourceId());
        else
        {
            /* Acquire existing profile names: */
            const QVector<QString> profileNames = m_comCloudProvider.GetProfileNames();
            /* Show error message if necessary: */
            if (!m_comCloudProvider.isOk())
                msgCenter().cannotAcquireCloudProviderParameter(m_comCloudProvider);
            else
            {
                /* Iterate through existing profile names: */
                foreach (const QString &strProfileName, profileNames)
                {
                    /* Skip if we have nothing to show (wtf happened?): */
                    if (strProfileName.isEmpty())
                        continue;

                    /* Compose item, fill it's data: */
                    m_pAccountComboBox->addItem(strProfileName);
                    m_pAccountComboBox->setItemData(m_pAccountComboBox->count() - 1, strProfileName, AccountData_ProfileName);
                }
            }
        }

        /* Set previous/default item if possible: */
        int iNewIndex = -1;
        if (   iNewIndex == -1
            && !strOldData.isNull())
            iNewIndex = m_pAccountComboBox->findData(strOldData, AccountData_ProfileName);
        if (   iNewIndex == -1
            && m_pAccountComboBox->count() > 0)
            iNewIndex = 0;
        if (iNewIndex != -1)
            m_pAccountComboBox->setCurrentIndex(iNewIndex);
    }

    /* Unblock signals after update: */
    m_pAccountComboBox->blockSignals(false);
}

void UIWizardImportAppPage1::populateAccountProperties()
{
    /* Clear table initially: */
    m_pAccountPropertyTable->clear();
    m_pAccountPropertyTable->setRowCount(0);
    m_pAccountPropertyTable->setColumnCount(0);
    /* Clear Cloud Profile: */
    m_comCloudProfile = CCloudProfile();

    /* If both provider and profile chosen: */
    if (!m_comCloudProvider.isNull() && !profileName().isNull())
    {
        /* Acquire Cloud Profile: */
        m_comCloudProfile = m_comCloudProvider.GetProfileByName(profileName());
        /* Show error message if necessary: */
        if (!m_comCloudProvider.isOk())
            msgCenter().cannotFindCloudProfile(m_comCloudProvider, profileName());
        else
        {
            /* Acquire properties: */
            QVector<QString> keys;
            QVector<QString> values;
            values = m_comCloudProfile.GetProperties(QString(), keys);
            /* Show error message if necessary: */
            if (!m_comCloudProfile.isOk())
                msgCenter().cannotAcquireCloudProfileParameter(m_comCloudProfile);
            else
            {
                /* Configure table: */
                m_pAccountPropertyTable->setRowCount(keys.size());
                m_pAccountPropertyTable->setColumnCount(2);

                /* Push acquired keys/values to data fields: */
                for (int i = 0; i < m_pAccountPropertyTable->rowCount(); ++i)
                {
                    /* Create key item: */
                    QTableWidgetItem *pItemK = new QTableWidgetItem(keys.at(i));
                    if (pItemK)
                    {
                        /* Non-editable for sure, but non-selectable? */
                        pItemK->setFlags(pItemK->flags() & ~Qt::ItemIsEditable);
                        pItemK->setFlags(pItemK->flags() & ~Qt::ItemIsSelectable);

                        /* Use non-translated description as tool-tip: */
                        const QString strToolTip = m_comCloudProvider.GetPropertyDescription(keys.at(i));
                        /* Show error message if necessary: */
                        if (!m_comCloudProfile.isOk())
                            msgCenter().cannotAcquireCloudProfileParameter(m_comCloudProfile);
                        else
                            pItemK->setData(Qt::UserRole, strToolTip);

                        /* Insert into table: */
                        m_pAccountPropertyTable->setItem(i, 0, pItemK);
                    }

                    /* Create value item: */
                    QTableWidgetItem *pItemV = new QTableWidgetItem(values.at(i));
                    if (pItemV)
                    {
                        /* Non-editable for sure, but non-selectable? */
                        pItemV->setFlags(pItemV->flags() & ~Qt::ItemIsEditable);
                        pItemV->setFlags(pItemV->flags() & ~Qt::ItemIsSelectable);

                        /* Use the value as tool-tip, there can be quite long values: */
                        const QString strToolTip = values.at(i);
                        pItemV->setToolTip(strToolTip);

                        /* Insert into table: */
                        m_pAccountPropertyTable->setItem(i, 1, pItemV);
                    }
                }

                /* Update table tool-tips: */
                updateAccountPropertyTableToolTips();

                /* Adjust the table: */
                adjustAccountPropertyTable();
            }
        }
    }
}

void UIWizardImportAppPage1::updatePageAppearance()
{
    /* Update page appearance according to chosen source: */
    m_pStackedLayout->setCurrentIndex((int)isSourceCloudOne());
}

void UIWizardImportAppPage1::updateSourceComboToolTip()
{
    const int iCurrentIndex = m_pSourceComboBox->currentIndex();
    const QString strCurrentToolTip = m_pSourceComboBox->itemData(iCurrentIndex, Qt::ToolTipRole).toString();
    AssertMsg(!strCurrentToolTip.isEmpty(), ("Data not found!"));
    m_pSourceComboBox->setToolTip(strCurrentToolTip);
}

void UIWizardImportAppPage1::updateAccountPropertyTableToolTips()
{
    /* Iterate through all the key items: */
    for (int i = 0; i < m_pAccountPropertyTable->rowCount(); ++i)
    {
        /* Acquire current key item: */
        QTableWidgetItem *pItemK = m_pAccountPropertyTable->item(i, 0);
        if (pItemK)
        {
            const QString strToolTip = pItemK->data(Qt::UserRole).toString();
            pItemK->setToolTip(QApplication::translate("UIWizardImportAppPageBasic1", strToolTip.toUtf8().constData()));
        }
    }
}

void UIWizardImportAppPage1::adjustAccountPropertyTable()
{
    /* Disable last column stretching temporary: */
    m_pAccountPropertyTable->horizontalHeader()->setStretchLastSection(false);

    /* Resize both columns to contents: */
    m_pAccountPropertyTable->resizeColumnsToContents();
    /* Then acquire full available width: */
    const int iFullWidth = m_pAccountPropertyTable->viewport()->width();
    /* First column should not be less than it's minimum size, last gets the rest: */
    const int iMinimumWidth0 = qMin(m_pAccountPropertyTable->horizontalHeader()->sectionSize(0), iFullWidth / 2);
    m_pAccountPropertyTable->horizontalHeader()->resizeSection(0, iMinimumWidth0);

    /* Enable last column stretching again: */
    m_pAccountPropertyTable->horizontalHeader()->setStretchLastSection(true);
}

void UIWizardImportAppPage1::setSource(const QString &strSource)
{
    const int iIndex = m_pSourceComboBox->findData(strSource, SourceData_ShortName);
    AssertMsg(iIndex != -1, ("Data not found!"));
    m_pSourceComboBox->setCurrentIndex(iIndex);
}

QString UIWizardImportAppPage1::source() const
{
    const int iIndex = m_pSourceComboBox->currentIndex();
    return m_pSourceComboBox->itemData(iIndex, SourceData_ShortName).toString();
}

bool UIWizardImportAppPage1::isSourceCloudOne(int iIndex /* = -1 */) const
{
    if (iIndex == -1)
        iIndex = m_pSourceComboBox->currentIndex();
    return m_pSourceComboBox->itemData(iIndex, SourceData_IsItCloudFormat).toBool();
}

QUuid UIWizardImportAppPage1::sourceId() const
{
    const int iIndex = m_pSourceComboBox->currentIndex();
    return m_pSourceComboBox->itemData(iIndex, SourceData_ID).toUuid();
}

QString UIWizardImportAppPage1::profileName() const
{
    const int iIndex = m_pAccountComboBox->currentIndex();
    return m_pAccountComboBox->itemData(iIndex, AccountData_ProfileName).toString();
}

CCloudProfile UIWizardImportAppPage1::profile() const
{
    return m_comCloudProfile;
}


/*********************************************************************************************************************************
*   Class UIWizardImportAppPageBasic1 implementation.                                                                            *
*********************************************************************************************************************************/

UIWizardImportAppPageBasic1::UIWizardImportAppPageBasic1(bool fImportFromOCIByDefault)
    : UIWizardImportAppPage1(fImportFromOCIByDefault)
    , m_pLabel(0)
{
    /* Create main layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    if (pMainLayout)
    {
        /* Create label: */
        m_pLabel = new QIRichTextLabel(this);
        if (m_pLabel)
        {
            /* Add into layout: */
            pMainLayout->addWidget(m_pLabel);
        }

        /* Create source layout: */
        m_pSourceLayout = new QGridLayout;
        if (m_pSourceLayout)
        {
            /* Create source label: */
            m_pSourceLabel = new QLabel(this);
            if (m_pSourceLabel)
            {
                m_pSourceLabel->hide();
                m_pSourceLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
                m_pSourceLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

                /* Add into layout: */
                m_pSourceLayout->addWidget(m_pSourceLabel, 0, 0);
            }

            /* Create source selector: */
            m_pSourceComboBox = new QIComboBox(this);
            if (m_pSourceComboBox)
            {
                m_pSourceLabel->setBuddy(m_pSourceComboBox);
                m_pSourceComboBox->hide();

                /* Add into layout: */
                m_pSourceLayout->addWidget(m_pSourceComboBox, 0, 1);
            }

            /* Add into layout: */
            pMainLayout->addLayout(m_pSourceLayout);
        }

        /* Create stacked layout: */
        m_pStackedLayout = new QStackedLayout;
        if (m_pStackedLayout)
        {
            /* Create local container: */
            QWidget *pLocalContainer = new QWidget(this);
            if (pLocalContainer)
            {
                /* Create local container layout: */
                QVBoxLayout *pLocalContainerLayout = new QVBoxLayout(pLocalContainer);
                if (pLocalContainerLayout)
                {
                    pLocalContainerLayout->setContentsMargins(0, 0, 0, 0);
                    pLocalContainerLayout->setSpacing(0);

                    /* Create file-path selector: */
                    m_pFileSelector = new UIEmptyFilePathSelector(this);
                    if (m_pFileSelector)
                    {
                        m_pFileSelector->setHomeDir(vboxGlobal().documentsPath());
                        m_pFileSelector->setMode(UIEmptyFilePathSelector::Mode_File_Open);
                        m_pFileSelector->setButtonPosition(UIEmptyFilePathSelector::RightPosition);
                        m_pFileSelector->setEditable(true);

                        /* Add into layout: */
                        pLocalContainerLayout->addWidget(m_pFileSelector);
                    }

                    /* Add stretch: */
                    pLocalContainerLayout->addStretch();
                }

                /* Add into layout: */
                m_pStackedLayout->addWidget(pLocalContainer);
            }

            /* Create cloud container: */
            QWidget *pCloudContainer = new QWidget(this);
            if (pCloudContainer)
            {
                /* Create cloud container layout: */
                m_pCloudContainerLayout = new QGridLayout(pCloudContainer);
                if (m_pCloudContainerLayout)
                {
                    m_pCloudContainerLayout->setContentsMargins(0, 0, 0, 0);

                    /* Create account label: */
                    m_pAccountLabel = new QLabel;
                    if (m_pAccountLabel)
                    {
                        m_pAccountLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

                        /* Add into layout: */
                        m_pCloudContainerLayout->addWidget(m_pAccountLabel, 0, 0);
                    }

                    /* Create sub-layout: */
                    QHBoxLayout *pSubLayout = new QHBoxLayout;
                    if (pSubLayout)
                    {
                        pSubLayout->setContentsMargins(0, 0, 0, 0);
                        pSubLayout->setSpacing(1);

                        /* Create account combo-box: */
                        m_pAccountComboBox = new QComboBox;
                        if (m_pAccountComboBox)
                        {
                            m_pAccountLabel->setBuddy(m_pAccountComboBox);

                            /* Add into layout: */
                            pSubLayout->addWidget(m_pAccountComboBox);
                        }

                        /* Create account tool-button: */
                        m_pAccountToolButton = new QIToolButton;
                        if (m_pAccountToolButton)
                        {
                            m_pAccountToolButton->setIcon(UIIconPool::iconSet(":/cloud_profile_manager_16px.png",
                                                                              ":/cloud_profile_manager_disabled_16px.png"));

                            /* Add into layout: */
                            pSubLayout->addWidget(m_pAccountToolButton);
                        }

                        /* Add into layout: */
                        m_pCloudContainerLayout->addLayout(pSubLayout, 0, 1);
                    }

                    /* Create profile property table: */
                    m_pAccountPropertyTable = new QTableWidget;
                    if (m_pAccountPropertyTable)
                    {
                        const QFontMetrics fm(m_pAccountPropertyTable->font());
                        const int iFontWidth = fm.width('x');
                        const int iTotalWidth = 50 * iFontWidth;
                        const int iFontHeight = fm.height();
                        const int iTotalHeight = 4 * iFontHeight;
                        m_pAccountPropertyTable->setMinimumSize(QSize(iTotalWidth, iTotalHeight));
//                        m_pAccountPropertyTable->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
                        m_pAccountPropertyTable->setAlternatingRowColors(true);
                        m_pAccountPropertyTable->horizontalHeader()->setVisible(false);
                        m_pAccountPropertyTable->verticalHeader()->setVisible(false);
                        m_pAccountPropertyTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

                        /* Add into layout: */
                        m_pCloudContainerLayout->addWidget(m_pAccountPropertyTable, 1, 1);
                    }
                }

                /* Add into layout: */
                m_pStackedLayout->addWidget(pCloudContainer);
            }

            /* Add into layout: */
            pMainLayout->addLayout(m_pStackedLayout);
        }

        /* Add vertical stretch finally: */
        pMainLayout->addStretch();
    }

    /* Populate sources: */
    populateSources();
    /* Populate accounts: */
    populateAccounts();
    /* Populate account properties: */
    populateAccountProperties();

    /* Setup connections: */
    if (gpManager)
        connect(gpManager, &UIVirtualBoxManager::sigCloudProfileManagerChange,
                this, &UIWizardImportAppPageBasic1::sltHandleSourceChange);
    connect(m_pSourceComboBox, static_cast<void(QIComboBox::*)(int)>(&QIComboBox::activated),
            this, &UIWizardImportAppPageBasic1::sltHandleSourceChange);
    connect(m_pFileSelector, &UIEmptyFilePathSelector::pathChanged,
            this, &UIWizardImportAppPageBasic1::completeChanged);
    connect(m_pAccountComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &UIWizardImportAppPageBasic1::sltHandleAccountComboChange);
    connect(m_pAccountToolButton, &QIToolButton::clicked,
            this, &UIWizardImportAppPageBasic1::sltHandleAccountButtonClick);

    /* Register fields: */
    registerField("source", this, "source");
    registerField("isSourceCloudOne", this, "isSourceCloudOne");
    registerField("profile", this, "profile");
}

bool UIWizardImportAppPageBasic1::event(QEvent *pEvent)
{
    /* Handle known event types: */
    switch (pEvent->type())
    {
        case QEvent::Show:
        case QEvent::Resize:
        {
            /* Adjust profile property table: */
            adjustAccountPropertyTable();
            break;
        }
        default:
            break;
    }

    /* Call to base-class: */
    return UIWizardPage::event(pEvent);
}

void UIWizardImportAppPageBasic1::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardImportApp::tr("Appliance to import"));

    /* Translate label: */
    m_pLabel->setText(UIWizardImportApp::tr("<p>VirtualBox currently supports importing appliances "
                                            "saved in the Open Virtualization Format (OVF). "
                                            "To continue, select the file to import below.</p>"));

    /* Translate source label: */
    m_pSourceLabel->setText(tr("&Source:"));
    /* Translate hardcoded values of Source combo-box: */
    m_pSourceComboBox->setItemText(0, UIWizardImportApp::tr("Local File System"));
    m_pSourceComboBox->setItemData(0, UIWizardImportApp::tr("Import from local file system."), Qt::ToolTipRole);
    /* Translate received values of Source combo-box.
     * We are enumerating starting from 0 for simplicity: */
    for (int i = 0; i < m_pSourceComboBox->count(); ++i)
        if (isSourceCloudOne(i))
        {
            m_pSourceComboBox->setItemText(i, m_pSourceComboBox->itemData(i, SourceData_Name).toString());
            m_pSourceComboBox->setItemData(i, UIWizardImportApp::tr("Import from cloud service provider."), Qt::ToolTipRole);
        }

    /* Translate file selector: */
    m_pFileSelector->setChooseButtonToolTip(UIWizardImportApp::tr("Choose a virtual appliance file to import..."));
    m_pFileSelector->setFileDialogTitle(UIWizardImportApp::tr("Please choose a virtual appliance file to import"));
    m_pFileSelector->setFileFilters(UIWizardImportApp::tr("Open Virtualization Format (%1)").arg("*.ova *.ovf"));

    /* Translate Account label: */
    m_pAccountLabel->setText(UIWizardImportApp::tr("&Account:"));

    /* Adjust label widths: */
    QList<QWidget*> labels;
    labels << m_pSourceLabel;
    labels << m_pAccountLabel;
    int iMaxWidth = 0;
    foreach (QWidget *pLabel, labels)
        iMaxWidth = qMax(iMaxWidth, pLabel->minimumSizeHint().width());
    m_pSourceLayout->setColumnMinimumWidth(0, iMaxWidth);
    m_pCloudContainerLayout->setColumnMinimumWidth(0, iMaxWidth);

    /* Update page appearance: */
    updatePageAppearance();

    /* Update tool-tips: */
    updateSourceComboToolTip();
    updateAccountPropertyTableToolTips();
}

void UIWizardImportAppPageBasic1::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardImportAppPageBasic1::isComplete() const
{
    bool fResult = true;

    /* Check appliance settings: */
    if (fResult)
    {
        const bool fOVF = !isSourceCloudOne();
        const bool fCSP = !fOVF;

        fResult =    (   fOVF
                      && VBoxGlobal::hasAllowedExtension(m_pFileSelector->path().toLower(), OVFFileExts)
                      && QFile::exists(m_pFileSelector->path()))
                  || (   fCSP
                      && !m_comCloudProfile.isNull());
    }

    return fResult;
}

bool UIWizardImportAppPageBasic1::validatePage()
{
    /* Get import appliance widget: */
    ImportAppliancePointer pImportApplianceWidget = field("applianceWidget").value<ImportAppliancePointer>();
    AssertMsg(!pImportApplianceWidget.isNull(), ("Appliance Widget is not set!\n"));

    /* If file name was changed: */
    if (m_pFileSelector->isModified())
    {
        /* Check if set file contains valid appliance: */
        if (!pImportApplianceWidget->setFile(m_pFileSelector->path()))
            return false;
        /* Reset the modified bit afterwards: */
        m_pFileSelector->resetModified();
    }

    /* If we have a valid ovf proceed to the appliance settings page: */
    return pImportApplianceWidget->isValid();
}

void UIWizardImportAppPageBasic1::sltHandleSourceChange()
{
    /* Update tool-tip: */
    updateSourceComboToolTip();

    /* Refresh required settings: */
    updatePageAppearance();
    populateAccounts();
    populateAccountProperties();
    emit completeChanged();
}

void UIWizardImportAppPageBasic1::sltHandleAccountComboChange()
{
    /* Refresh required settings: */
    populateAccountProperties();
}

void UIWizardImportAppPageBasic1::sltHandleAccountButtonClick()
{
    /* Open Cloud Profile Manager: */
    if (gpManager)
        gpManager->openCloudProfileManager();
}
