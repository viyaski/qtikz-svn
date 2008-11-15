/***************************************************************************
 *   Copyright (C) 2007-2008 by Glad Deschrijver                           *
 *   Glad.Deschrijver@UGent.be                                             *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QAction>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QGraphicsPixmapItem>
#include <QLineEdit>
#include <QMenu>
#include <QScrollBar>
#include <QSettings>
#include <QToolBar>

#include "tikzpreview.h"

TikzPreview::TikzPreview(QWidget *parent)
    : QGraphicsView(parent)
{
	m_tikzScene = new QGraphicsScene(this);
	m_tikzPixmapItem = m_tikzScene->addPixmap(QPixmap());
	setScene(m_tikzScene);
	setDragMode(QGraphicsView::ScrollHandDrag);
	setWhatsThis("<p>" + tr("Here the preview image of "
	    "your TikZ code is shown.  You can zoom in and out, and you "
	    "can scroll the image by dragging it.") + "</p>");

	m_tikzPdfDoc = 0;
	m_currentPage = 0;

	QSettings settings;
	m_zoomFactor = settings.value("ZoomFactor", 1).toDouble();
	m_oldZoomFactor = m_zoomFactor;
	m_minZoomFactor = 0.1;
	m_maxZoomFactor = 6;
	m_isZooming = false;

	createActions();
	createViewToolBar();
}

TikzPreview::~TikzPreview()
{
	QSettings settings;
	settings.setValue("ZoomFactor", m_zoomFactor);
}

void TikzPreview::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu = new QMenu(this);
	menu->addActions(getActions());
	menu->exec(event->globalPos());
	menu->deleteLater();
}

QSize TikzPreview::sizeHint() const
{
	return QSize(250, 200);
}

void TikzPreview::centerView()
{
	m_isZooming = true;
}

void TikzPreview::paintEvent(QPaintEvent *event)
{
	if (m_isZooming)
	{
		/* center the viewport on the same object in the image
		 * that was previously the center (in order to avoid
		 * flicker, this must be done here) */
		const qreal zoomFraction = m_zoomFactor / m_oldZoomFactor;
		setSceneRect(m_tikzScene->itemsBoundingRect());
		centerOn((horizontalScrollBar()->value() + viewport()->width() / 2) * zoomFraction,
			(verticalScrollBar()->value() + viewport()->height() / 2) * zoomFraction);
		m_oldZoomFactor = m_zoomFactor; // m_oldZoomFactor must be set here and not in the zoom functions below in order to avoid skipping some steps when the user zooms fast
		m_isZooming = false;
	}
	QGraphicsView::paintEvent(event);
}

void TikzPreview::setZoomFactor()
{
	m_zoomFactor = m_zoomCombo->lineEdit()->text().remove(QRegExp("[^\\d\\.]*")).toDouble() / 100;

	if (m_zoomFactor < m_minZoomFactor)
		m_zoomFactor = m_minZoomFactor;
	else if (m_zoomFactor > m_maxZoomFactor)
		m_zoomFactor = m_maxZoomFactor;

	showPdfPage();
	centerView();
}

void TikzPreview::zoomIn()
{
	if (m_zoomFactor < 0.999999)
		m_zoomFactor += 0.1;
	else if (m_zoomFactor < 1.999999)
		m_zoomFactor += 0.2;
	else
		m_zoomFactor += 0.5;
	if (m_zoomFactor > m_maxZoomFactor)
		m_zoomFactor = m_maxZoomFactor;
	showPdfPage();
	centerView();
}

void TikzPreview::zoomOut()
{
	if (m_zoomFactor < 1.000001)
		m_zoomFactor -= 0.1;
	else if (m_zoomFactor < 2.000001)
		m_zoomFactor -= 0.2;
	else
		m_zoomFactor -= 0.5;
	if (m_zoomFactor < m_minZoomFactor)
		m_zoomFactor = m_minZoomFactor;
	showPdfPage();
	centerView();
}

void TikzPreview::showPreviousPage()
{
	if (m_currentPage > 0)
		--m_currentPage;
	m_previousPageAction->setEnabled(m_currentPage > 0);
	m_nextPageAction->setEnabled(m_currentPage < m_tikzPdfDoc->numPages() - 1);
	showPdfPage();
}

void TikzPreview::showNextPage()
{
	if (m_currentPage < m_tikzPdfDoc->numPages() - 1)
		++m_currentPage;
	m_previousPageAction->setEnabled(m_currentPage > 0);
	m_nextPageAction->setEnabled(m_currentPage < m_tikzPdfDoc->numPages() - 1);
	showPdfPage();
}

void TikzPreview::showPdfPage()
{
	m_zoomCombo->lineEdit()->setText(QString::number(m_zoomFactor * 100) + "%");

	if (!m_tikzPdfDoc || m_tikzPdfDoc->numPages() < 1) return;

	Poppler::Page *pdfPage = m_tikzPdfDoc->page(m_currentPage);
	m_tikzPixmapItem->setPixmap(QPixmap::fromImage(pdfPage->renderToImage(m_zoomFactor * 72, m_zoomFactor * 72)));
	m_tikzPixmapItem->update();
	delete pdfPage;
}

void TikzPreview::pixmapUpdated(Poppler::Document *tikzPdfDoc)
{
    if (!tikzPdfDoc)
        pixmapUpdatedEmpty();
    else
	{
		m_tikzPdfDoc = tikzPdfDoc;
        pixmapUpdated();
	}
}

void TikzPreview::pixmapUpdatedEmpty()
{
	m_tikzPixmapItem->setPixmap(QPixmap());
	m_tikzPixmapItem->update();
}

void TikzPreview::pixmapUpdated()
{
	m_tikzPdfDoc->setRenderBackend(Poppler::Document::SplashBackend);
	m_tikzPdfDoc->setRenderHint(Poppler::Document::Antialiasing, true);
	m_tikzPdfDoc->setRenderHint(Poppler::Document::TextAntialiasing, true);
	const int numOfPages = m_tikzPdfDoc->numPages();

	bool visible = false;
	if (numOfPages > 1)
		visible = true;
	m_viewToolBar->actions().at(m_viewToolBar->actions().indexOf(m_previousPageAction)-1)->setVisible(visible); // show separator
	m_previousPageAction->setVisible(visible);
	m_nextPageAction->setVisible(visible);

	if (m_currentPage >= numOfPages)
		m_currentPage = 0;

	showPdfPage();
	centerView(); // adjust viewport when new objects are added to the tikz picture
}

void TikzPreview::createActions()
{
	m_zoomInAction = new QAction(QIcon(":/images/zoom-in.png"), tr("Zoom &In"), this);
	m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
	m_zoomInAction->setStatusTip(tr("Zoom preview in"));
	m_zoomInAction->setWhatsThis("<p>" + tr("Zoom preview in by a predetermined factor.") + "</p>");
	connect(m_zoomInAction, SIGNAL(triggered()), this, SLOT(zoomIn()));

	m_zoomOutAction = new QAction(QIcon(":/images/zoom-out.png"), tr("Zoom &Out"), this);
	m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
	m_zoomOutAction->setStatusTip(tr("Zoom preview out"));
	m_zoomOutAction->setWhatsThis("<p>" + tr("Zoom preview out by a predetermined factor.") + "</p>");
	connect(m_zoomOutAction, SIGNAL(triggered()), this, SLOT(zoomOut()));

	m_previousPageAction = new QAction(QIcon(":/images/go-previous.png"), tr("&Previous image"), this);
	m_previousPageAction->setShortcut(tr("Alt+Left", "Go to previous page"));
	m_previousPageAction->setStatusTip(tr("Show previous image in preview"));
	m_previousPageAction->setWhatsThis("<p>" + tr("Show the preview of the previous tikzpicture in the TikZ code.") + "</p>");
	connect(m_previousPageAction, SIGNAL(triggered()), this, SLOT(showPreviousPage()));

	m_nextPageAction = new QAction(QIcon(":/images/go-next.png"), tr("&Next image"), this);
	m_nextPageAction->setShortcut(tr("Alt+Right", "Go to next page"));
	m_nextPageAction->setStatusTip(tr("Show next image in preview"));
	m_nextPageAction->setWhatsThis("<p>" + tr("Show the preview of the next tikzpicture in the TikZ code.") + "</p>");
	connect(m_nextPageAction, SIGNAL(triggered()), this, SLOT(showNextPage()));

	m_previousPageAction->setVisible(false);
	m_previousPageAction->setEnabled(false);
	m_nextPageAction->setVisible(false);
	m_nextPageAction->setEnabled(true);
}

QList<QAction*> TikzPreview::getActions()
{
	QList<QAction*> actions;
	actions << m_zoomInAction << m_zoomOutAction;
	QAction *action = new QAction(this);
	action->setSeparator(true);
	actions << action;
	actions << m_previousPageAction << m_nextPageAction;
	return actions;
}

void TikzPreview::createViewToolBar()
{
	QStringList zoomSizesList;
	zoomSizesList << "12.50%" << "25%" << "50%" << "75%" << "100%" << "125%" << "150%" << "200%" << "250%" << "300%";
	m_zoomCombo = new QComboBox;
	m_zoomCombo->setEditable(true);
	m_zoomCombo->setToolTip(tr("Select or insert zoom factor here"));
	m_zoomCombo->setWhatsThis("<p>" + tr("Select the zoom factor here.  "
	    "Alternatively, you can also introduce a zoom factor and "
	    "press Enter.") + "</p>");
	m_zoomCombo->insertItems(0, zoomSizesList);
	const QString zoomText = QString::number(m_zoomFactor * 100) + "%";
	m_zoomCombo->setCurrentIndex(m_zoomCombo->findText(zoomText));
	m_zoomCombo->lineEdit()->setText(zoomText);
	connect(m_zoomCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setZoomFactor()));
	connect(m_zoomCombo->lineEdit(), SIGNAL(returnPressed()), this, SLOT(setZoomFactor()));

	m_viewToolBar = new QToolBar(tr("View"));
	m_viewToolBar->setObjectName("ViewToolBar");
	m_viewToolBar->addAction(m_zoomInAction);
	m_viewToolBar->addWidget(m_zoomCombo);
	m_viewToolBar->addAction(m_zoomOutAction);
	m_viewToolBar->addSeparator();
	m_viewToolBar->addAction(m_previousPageAction);
	m_viewToolBar->addAction(m_nextPageAction);
	m_viewToolBar->actions().at(m_viewToolBar->actions().indexOf(m_previousPageAction)-1)->setVisible(false);
}

QToolBar *TikzPreview::getViewToolBar()
{
	return m_viewToolBar;
}

QPixmap TikzPreview::getPixmap() const
{
	return m_tikzPixmapItem->pixmap();
}

void TikzPreview::wheelEvent(QWheelEvent *event)
{
	if (event->modifiers() == Qt::ControlModifier)
	{
		if (event->delta() > 0)
			zoomIn();
		else
			zoomOut();
	}
	else
		QGraphicsView::wheelEvent(event);
}
