/***************************************************************************
 *   Copyright (C) 2009 by Jean-Baptiste Mardelle (jb@kdenlive.org)        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "dvdwizardvob.h"
#include "kthumb.h"
#include "timecode.h"

#include <mlt++/Mlt.h>

#include <KUrlRequester>
#include <KDebug>
#include <KStandardDirs>
#include <KFileItem>
#include <KFileDialog>

#include <QHBoxLayout>
#include <QDomDocument>
#include <QTreeWidgetItem>

DvdWizardVob::DvdWizardVob(const QString &profile, QWidget *parent) :
        QWizardPage(parent)
{
    m_view.setupUi(this);
    m_view.intro_vob->setEnabled(false);
    m_view.intro_vob->setFilter("video/mpeg");
    m_view.button_add->setIcon(KIcon("document-new"));
    m_view.button_delete->setIcon(KIcon("edit-delete"));
    m_view.button_up->setIcon(KIcon("go-up"));
    m_view.button_down->setIcon(KIcon("go-down"));
    connect(m_view.use_intro, SIGNAL(toggled(bool)), m_view.intro_vob, SLOT(setEnabled(bool)));
    connect(m_view.button_add, SIGNAL(clicked()), this, SLOT(slotAddVobFile()));
    connect(m_view.button_delete, SIGNAL(clicked()), this, SLOT(slotDeleteVobFile()));
    connect(m_view.button_up, SIGNAL(clicked()), this, SLOT(slotItemUp()));
    connect(m_view.button_down, SIGNAL(clicked()), this, SLOT(slotItemDown()));
    connect(m_view.vobs_list, SIGNAL(itemSelectionChanged()), this, SLOT(slotCheckVobList()));
    m_view.vobs_list->setIconSize(QSize(60, 45));

    if (KStandardDirs::findExe("dvdauthor").isEmpty()) m_errorMessage.append(i18n("<strong>Program %1 is required for the DVD wizard.", i18n("dvdauthor")));
    if (KStandardDirs::findExe("mkisofs").isEmpty()) m_errorMessage.append(i18n("<strong>Program %1 is required for the DVD wizard.", i18n("mkisofs")));
    if (m_errorMessage.isEmpty()) m_view.error_message->setVisible(false);
    else m_view.error_message->setText(m_errorMessage);

    m_view.dvd_profile->addItems(QStringList() << i18n("PAL") << i18n("NTSC"));
    if (profile == "dv_ntsc" || profile == "dv_ntsc_wide") {
        m_view.dvd_profile->setCurrentIndex(1);
    }
    connect(m_view.dvd_profile, SIGNAL(activated(int)), this, SLOT(changeFormat()));
    m_view.vobs_list->header()->setStretchLastSection(false);
    m_view.vobs_list->header()->setResizeMode(0, QHeaderView::Stretch);
    m_view.vobs_list->header()->setResizeMode(1, QHeaderView::Custom);
    m_view.vobs_list->header()->setResizeMode(2, QHeaderView::Custom);

#if KDE_IS_VERSION(4,2,0)
    m_capacityBar = new KCapacityBar(KCapacityBar::DrawTextInline, this);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_capacityBar);
    m_view.size_box->setLayout(layout);
#else
    m_view.size_box->setHidden(true);
#endif
    slotCheckVobList();
}

DvdWizardVob::~DvdWizardVob()
{
#if KDE_IS_VERSION(4,2,0)
    delete m_capacityBar;
#endif
}


void DvdWizardVob::slotAddVobFile(KUrl url)
{
    if (url.isEmpty()) url = KFileDialog::getOpenUrl(KUrl("kfiledialog:///projectfolder"), "video/mpeg", this, i18n("Add new video file"));
    if (url.isEmpty()) return;
    QFile f(url.path());
    qint64 fileSize = f.size();
    QString profilename;
    if (m_view.dvd_profile->currentIndex() == 0) profilename = "dv_pal";
    else profilename = "dv_ntsc";
    Mlt::Profile profile((char*) profilename.data());
    QTreeWidgetItem *item = new QTreeWidgetItem(m_view.vobs_list, QStringList() << url.path() << QString() << KIO::convertSize(fileSize));
    item->setData(0, Qt::UserRole, fileSize);
    item->setIcon(0, KIcon("video-x-generic"));
    if (QFile::exists(url.path() + ".dvdchapter")) {
        // insert chapters as children
        QFile file(url.path() + ".dvdchapter");
        if (file.open(QIODevice::ReadOnly)) {
            QDomDocument doc;
            doc.setContent(&file);
            file.close();
            QDomNodeList chapters = doc.elementsByTagName("chapter");
            QStringList chaptersList;
            for (int j = 0; j < chapters.count(); j++) {
                QTreeWidgetItem *sub = new QTreeWidgetItem(item, QStringList() << QString::number(j) + " - " + chapters.at(j).toElement().attribute("title"));
                sub->setText(1, Timecode::getStringTimecode(chapters.at(j).toElement().attribute("time").toInt(), profile.fps()));
                sub->setData(1, Qt::UserRole, chapters.at(j).toElement().attribute("time").toInt());
            }
        }
    }

    QPixmap pix(60, 45);

    char *tmp = (char *) qstrdup(url.path().toUtf8().data());
    Mlt::Producer *producer = new Mlt::Producer(profile, tmp);
    delete[] tmp;

    if (producer->is_blank() == false) {
        pix = KThumb::getFrame(producer, 0, 60, 45);
        item->setIcon(0, pix);
        item->setText(1, Timecode::getStringTimecode(producer->get_playtime(), profile.fps()));
    }
    delete producer;

    slotCheckVobList();
}

void DvdWizardVob::changeFormat()
{
    int max = m_view.vobs_list->topLevelItemCount();
    QString profilename;
    if (m_view.dvd_profile->currentIndex() == 0) profilename = "dv_pal";
    else profilename = "dv_ntsc";
    Mlt::Profile profile((char*) profilename.data());
    QPixmap pix(180, 135);

    for (int i = 0; i < max; i++) {
        QTreeWidgetItem *item = m_view.vobs_list->topLevelItem(i);
        char *tmp = (char *) qstrdup(item->text(0).toUtf8().data());
        Mlt::Producer *producer = new Mlt::Producer(profile, tmp);
        delete[] tmp;

        if (producer->is_blank() == false) {
            //pix = KThumb::getFrame(producer, 0, 180, 135);
            //item->setIcon(0, pix);
            item->setText(1, Timecode::getStringTimecode(producer->get_playtime(), profile.fps()));
        }
        delete producer;
        int submax = item->childCount();
        for (int j = 0; j < submax; j++) {
            QTreeWidgetItem *subitem = item->child(j);
            subitem->setText(1, Timecode::getStringTimecode(subitem->data(1, Qt::UserRole).toInt(), profile.fps()));
        }
    }
    slotCheckVobList();
}

void DvdWizardVob::slotDeleteVobFile()
{
    QTreeWidgetItem *item = m_view.vobs_list->currentItem();
    if (item == NULL) return;
    delete item;
    slotCheckVobList();
}


// virtual
bool DvdWizardVob::isComplete() const
{
    if (!m_view.error_message->text().isEmpty()) return false;
    if (m_view.vobs_list->topLevelItemCount() == 0) return false;
    return true;
}

bool DvdWizardVob::useChapters() const
{
    return true; //m_view.use_chapters->isChecked();
}

void DvdWizardVob::setUrl(const QString &url)
{
    slotAddVobFile(KUrl(url));
}

QStringList DvdWizardVob::selectedUrls() const
{
    QStringList result;
    QString path;
    int max = m_view.vobs_list->topLevelItemCount();
    for (int i = 0; i < max; i++) {
        QTreeWidgetItem *item = m_view.vobs_list->topLevelItem(i);
        if (item) result.append(item->text(0));
    }
    return result;
}

QStringList DvdWizardVob::selectedTitles() const
{
    QStringList result;
    int max = m_view.vobs_list->topLevelItemCount();
    for (int i = 0; i < max; i++) {
        QTreeWidgetItem *item = m_view.vobs_list->topLevelItem(i);
        if (item) {
            result.append(item->text(0));
            int submax = item->childCount();
            for (int j = 0; j < submax; j++) {
                QTreeWidgetItem *subitem = item->child(j);
                result.append(subitem->text(0) + ' ' + subitem->text(1));
            }
        }
    }
    return result;
}

QStringList DvdWizardVob::chapter(int ix) const
{
    QStringList result;
    QTreeWidgetItem *item = m_view.vobs_list->topLevelItem(ix);
    if (item) {
        int submax = item->childCount();
        for (int j = 0; j < submax; j++) {
            QTreeWidgetItem *subitem = item->child(j);
            result.append(subitem->text(1));
        }
    }
    return result;
}

QStringList DvdWizardVob::selectedTargets() const
{
    QStringList result;
    int max = m_view.vobs_list->topLevelItemCount();
    for (int i = 0; i < max; i++) {
        QTreeWidgetItem *item = m_view.vobs_list->topLevelItem(i);
        if (item) {
            result.append("jump title " + QString::number(i + 1));
            int submax = item->childCount();
            for (int j = 0; j < submax; j++) {
                QTreeWidgetItem *subitem = item->child(j);
                result.append("jump title " + QString::number(i + 1) + " chapter " + QString::number(j + 1));
            }
        }
    }
    return result;
}


QString DvdWizardVob::introMovie() const
{
    if (!m_view.use_intro->isChecked()) return QString();
    return m_view.intro_vob->url().path();
}

void DvdWizardVob::slotCheckVobList()
{
    emit completeChanged();
    int max = m_view.vobs_list->topLevelItemCount();
    QTreeWidgetItem *item = m_view.vobs_list->currentItem();
    bool hasItem = true;
    if (item == NULL) hasItem = false;
    m_view.button_delete->setEnabled(hasItem);
    if (hasItem && m_view.vobs_list->indexOfTopLevelItem(item) == 0) m_view.button_up->setEnabled(false);
    else m_view.button_up->setEnabled(hasItem);
    if (hasItem && m_view.vobs_list->indexOfTopLevelItem(item) == max - 1) m_view.button_down->setEnabled(false);
    else m_view.button_down->setEnabled(hasItem);

#if KDE_IS_VERSION(4,2,0)
    qint64 totalSize = 0;
    for (int i = 0; i < max; i++) {
        item = m_view.vobs_list->topLevelItem(i);
        if (item) totalSize += (qint64) item->data(0, Qt::UserRole).toInt();
    }

    qint64 maxSize = (qint64) 47000 * 100000;
    m_capacityBar->setValue(100 * totalSize / maxSize);
    m_capacityBar->setText(KIO::convertSize(totalSize));
#endif
}

void DvdWizardVob::slotItemUp()
{
    QTreeWidgetItem *item = m_view.vobs_list->currentItem();
    if (item == NULL) return;
    int index = m_view.vobs_list->indexOfTopLevelItem(item);
    if (index == 0) return;
    m_view.vobs_list->insertTopLevelItem(index - 1, m_view.vobs_list->takeTopLevelItem(index));
}

void DvdWizardVob::slotItemDown()
{
    int max = m_view.vobs_list->topLevelItemCount();
    QTreeWidgetItem *item = m_view.vobs_list->currentItem();
    if (item == NULL) return;
    int index = m_view.vobs_list->indexOfTopLevelItem(item);
    if (index == max - 1) return;
    m_view.vobs_list->insertTopLevelItem(index + 1, m_view.vobs_list->takeTopLevelItem(index));
}

bool DvdWizardVob::isPal() const
{
    return m_view.dvd_profile->currentIndex() == 0;
}



