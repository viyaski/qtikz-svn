/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2010, 2011 by Glad Deschrijver        *
 *     <glad.deschrijver@gmail.com>                                        *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "tikzpreview.h"

#include <QtGui/QApplication>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QGraphicsPixmapItem>
#include <QtGui/QGraphicsProxyWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QScrollBar>
#include <QtCore/QSettings>
#include <QtGui/QToolBar>

#include <poppler-qt4.h>

#include "tikzpreviewrenderer.h"
#include "utils/action.h"
#include "utils/icon.h"
#include "utils/standardaction.h"
#include "utils/zoomaction.h"

TikzPreview::TikzPreview(QWidget *parent)
	: QGraphicsView(parent)
{
	m_tikzScene = new QGraphicsScene(this);
	m_tikzPixmapItem = m_tikzScene->addPixmap(QPixmap());
	setScene(m_tikzScene);
	setDragMode(QGraphicsView::ScrollHandDrag);
	m_tikzPixmapItem->setCursor(Qt::CrossCursor);
	setWhatsThis(tr("<p>Here the preview image of "
	                "your TikZ code is shown.  You can zoom in and out, and you "
	                "can scroll the image by dragging it.</p>"));

	m_tikzPdfDoc = 0;
	m_currentPage = 0;
	m_processRunning = false;
	m_pageSeparator = 0;

	QSettings settings(ORGNAME, APPNAME);
	settings.beginGroup("Preview");
	m_zoomFactor = settings.value("ZoomFactor", 1).toDouble();
	settings.endGroup();
	m_oldZoomFactor = -1;
	m_hasZoomed = false;

	createActions();
	createInformationLabel();

	m_tikzPreviewRenderer = new TikzPreviewRenderer();
	connect(this, SIGNAL(generatePreview(Poppler::Document*,qreal,int)), m_tikzPreviewRenderer, SLOT(generatePreview(Poppler::Document*,qreal,int)));
	connect(m_tikzPreviewRenderer, SIGNAL(showPreview(QImage,qreal)), this, SLOT(showPreview(QImage,qreal)));
}

TikzPreview::~TikzPreview()
{
	delete m_tikzPixmapItem;
	delete m_infoProxyWidget;
	delete m_tikzPreviewRenderer;

	QSettings settings(ORGNAME, APPNAME);
	settings.beginGroup("Preview");
	settings.setValue("ZoomFactor", m_zoomFactor);
	settings.endGroup();
}

void TikzPreview::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu = new QMenu(this);
	menu->addActions(actions());
	menu->exec(event->globalPos());
	menu->deleteLater();
}

QSize TikzPreview::sizeHint() const
{
	return QSize(250, 200);
}

/***************************************************************************/

void TikzPreview::createActions()
{
	m_zoomInAction = StandardAction::zoomIn(this, SLOT(zoomIn()), this);
	m_zoomOutAction = StandardAction::zoomOut(this, SLOT(zoomOut()), this);
	m_zoomInAction->setStatusTip(tr("Zoom preview in"));
	m_zoomOutAction->setStatusTip(tr("Zoom preview out"));
	m_zoomInAction->setWhatsThis(tr("<p>Zoom preview in by a predetermined factor.</p>"));
	m_zoomOutAction->setWhatsThis(tr("<p>Zoom preview out by a predetermined factor.</p>"));

	m_zoomToAction = new ZoomAction(Icon("zoom-original"), tr("&Zoom"), this, "zoom_to");
	m_zoomToAction->setZoomFactor(m_zoomFactor);
	connect(m_zoomToAction, SIGNAL(zoomFactorAdded(qreal)), this, SLOT(setZoomFactor(qreal)));

	m_previousPageAction = new Action(Icon("go-previous"), tr("&Previous image"), this, "view_previous_image");
	m_previousPageAction->setShortcut(tr("Alt+Left", "View|Go to previous page"));
	m_previousPageAction->setStatusTip(tr("Show previous image in preview"));
	m_previousPageAction->setWhatsThis(tr("<p>Show the preview of the previous tikzpicture in the TikZ code.</p>"));
	connect(m_previousPageAction, SIGNAL(triggered()), this, SLOT(showPreviousPage()));

	m_nextPageAction = new Action(Icon("go-next"), tr("&Next image"), this, "view_next_image");
	m_nextPageAction->setShortcut(tr("Alt+Right", "View|Go to next page"));
	m_nextPageAction->setStatusTip(tr("Show next image in preview"));
	m_nextPageAction->setWhatsThis(tr("<p>Show the preview of the next tikzpicture in the TikZ code.</p>"));
	connect(m_nextPageAction, SIGNAL(triggered()), this, SLOT(showNextPage()));

	m_previousPageAction->setVisible(false);
	m_previousPageAction->setEnabled(false);
	m_nextPageAction->setVisible(false);
	m_nextPageAction->setEnabled(true);
}

QList<QAction*> TikzPreview::actions()
{
	QList<QAction*> actions;
	actions << m_zoomInAction << m_zoomOutAction;
	QAction *action = new QAction(this);
	action->setSeparator(true);
	actions << action;
	actions << m_previousPageAction << m_nextPageAction;
	return actions;
}

QToolBar *TikzPreview::toolBar()
{
	QToolBar *viewToolBar = new QToolBar(tr("View"), this);
	viewToolBar->setObjectName("ViewToolBar");
	viewToolBar->addAction(m_zoomInAction);
	viewToolBar->addAction(m_zoomToAction);
	viewToolBar->addAction(m_zoomOutAction);
	m_pageSeparator = viewToolBar->addSeparator();
	m_pageSeparator->setVisible(false);
	viewToolBar->addAction(m_previousPageAction);
	viewToolBar->addAction(m_nextPageAction);
	return viewToolBar;
}

/***************************************************************************/

void TikzPreview::createInformationLabel()
{
#ifdef KTIKZ_USE_KDE
	const QPixmap infoPixmap = KIconLoader::global()->loadIcon("dialog-error",
	                           KIconLoader::Dialog, KIconLoader::SizeMedium);
#else
	const QPixmap infoPixmap = Icon("dialog-error").pixmap(QSize(32, 32));
#endif
	m_infoPixmapLabel = new QLabel;
	m_infoPixmapLabel->setPixmap(infoPixmap);

	m_infoLabel = new QLabel;
	m_infoLabel->setWordWrap(true);
	m_infoLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	m_infoWidget = new QFrame;
	m_infoWidget->setObjectName("infoWidget");
	m_infoWidget->setFrameShape(QFrame::Box);
/*
	m_infoWidget->setStyleSheet(QString(
	    ".QFrame {"
//	    "  background-color: palette(window);"
	    "  border-radius: 5px;"
	    "  border: 1px solid palette(dark);"
	    "}"
	    ".QLabel { color: palette(windowText); }"
	));
*/

	QPalette palette = qApp->palette();
	QColor backgroundColor = palette.window().color();
	QColor foregroundColor = palette.color(QPalette::Dark);
	backgroundColor.setAlpha(220);
	foregroundColor.setAlpha(150);
	palette.setBrush(QPalette::Window, backgroundColor);
	palette.setBrush(QPalette::WindowText, foregroundColor);
	m_infoWidget->setPalette(palette);

	palette = m_infoLabel->palette();
	foregroundColor = palette.windowText().color();
	palette.setBrush(QPalette::WindowText, foregroundColor);
	m_infoLabel->setPalette(palette);

	QHBoxLayout *infoLayout = new QHBoxLayout(m_infoWidget);
	infoLayout->setMargin(10);
	infoLayout->addWidget(m_infoPixmapLabel);
	infoLayout->addWidget(m_infoLabel);

	m_infoProxyWidget = m_tikzScene->addWidget(m_infoWidget);
	m_infoProxyWidget->setZValue(1);
//	m_infoProxyWidget->setVisible(false);
	m_tikzScene->removeItem(m_infoProxyWidget);
	m_infoWidgetAdded = false;

	m_infoPixmapLabel->setVisible(false);
}

/***************************************************************************/

void TikzPreview::paintEvent(QPaintEvent *event)
{
	// when m_infoWidget is visible, then it must be resized and
	// repositioned, this must be done here to do it successfully;
	// why m_infoWidget->resize() doesn't work in setInfoLabelText()
	// is beyond my understanding :-(
	if (m_infoWidgetAdded && m_infoWidget->isVisible())
	{
		if (m_infoPixmapLabel->isVisible())
		{
			m_infoWidget->resize(m_infoPixmapLabel->sizeHint().width()
			    + m_infoLabel->sizeHint().width() + 35,
			    qMax(m_infoPixmapLabel->sizeHint().height(), m_infoLabel->sizeHint().height()) + 25);
		}
		else
		{
			m_infoWidget->resize(m_infoLabel->sizeHint().width() + 25,
			    m_infoLabel->sizeHint().height() + 25);
		}
		centerInfoLabel();
		m_infoWidgetAdded = false;
	}

	if (m_hasZoomed)
	{
		setSceneRect(m_tikzScene->itemsBoundingRect()); // make sure that the scroll area is not bigger than the actual image
		m_hasZoomed = false;
		m_infoWidgetAdded = true; // dirty hack to force m_infoWidget to be recentered also when zooming
	}

	QGraphicsView::paintEvent(event);
}

/***************************************************************************/

void TikzPreview::setZoomFactor(qreal zoomFactor)
{
	m_zoomFactor = zoomFactor;
	if (m_zoomFactor == m_oldZoomFactor)
		return;

	m_zoomInAction->setEnabled(m_zoomFactor < m_zoomToAction->maxZoomFactor());
	m_zoomOutAction->setEnabled(m_zoomFactor > m_zoomToAction->minZoomFactor());

	showPdfPage();
}

void TikzPreview::zoomIn()
{
	m_zoomToAction->setZoomFactor(m_zoomFactor + (m_zoomFactor > 0.99 ?
	                                             (m_zoomFactor > 1.99 ? 0.5 : 0.2) : 0.1));
}

void TikzPreview::zoomOut()
{
	m_zoomToAction->setZoomFactor(m_zoomFactor - (m_zoomFactor > 1.01 ?
	                                             (m_zoomFactor > 2.01 ? 0.5 : 0.2) : 0.1));
}

/***************************************************************************/

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

void TikzPreview::showPreview(const QImage &tikzImage, qreal zoomFactor)
{
	// this slot is called when TikzPreviewRenderer has finished rendering
	// the current pdf page to tikzImage, so before we actually display
	// the image the old center point must be calculated and multiplied
	// by the quotient of the new and old zoom factor in order to obtain
	// the new center point of the image; the recentering itself is done
	// at the end of this function
	QPointF centerPoint(horizontalScrollBar()->value() + viewport()->width() * 0.5,
	                    verticalScrollBar()->value() + viewport()->height() * 0.5);
	const qreal zoomFraction = (m_oldZoomFactor > 0) ? zoomFactor / m_oldZoomFactor : 1;
	if (!centerPoint.isNull())
		centerPoint *= zoomFraction;
	m_oldZoomFactor = zoomFactor; // m_oldZoomFactor must be set here and not in the zoom functions in order to avoid skipping some steps when the user zooms fast
	m_hasZoomed = true;

	// display and center the preview image
	m_tikzPixmapItem->setPixmap(QPixmap::fromImage(tikzImage));
	centerOn(centerPoint);
}

void TikzPreview::showPdfPage()
{
	if (!m_tikzPdfDoc || m_tikzPdfDoc->numPages() < 1)
		return;

	if (!m_processRunning)
		emit generatePreview(m_tikzPdfDoc, m_zoomFactor, m_currentPage); // render the current pdf page to a QImage in TikzPreviewRenderer (in a different thread)
}

void TikzPreview::emptyPreview()
{
	m_tikzPdfDoc = 0;
	m_tikzCoordinates.clear();
	m_tikzPixmapItem->setPixmap(QPixmap());
	m_tikzPixmapItem->update();
//	m_infoWidget->setVisible(false);
	if (m_infoProxyWidget->scene() != 0) // remove error messages from view
		m_tikzScene->removeItem(m_infoProxyWidget);
	m_tikzScene->setSceneRect(0, 0, 1, 1); // remove scrollbars from view
	if (m_pageSeparator)
		m_pageSeparator->setVisible(false);
	m_previousPageAction->setVisible(false);
	m_nextPageAction->setVisible(false);
}

void TikzPreview::pixmapUpdated(Poppler::Document *tikzPdfDoc, const QList<qreal> &tikzCoordinates)
{
	m_tikzPdfDoc = tikzPdfDoc;
	m_tikzCoordinates = tikzCoordinates;

	if (!m_tikzPdfDoc)
	{
		emptyPreview();
		return;
	}

	m_tikzPdfDoc->setRenderBackend(Poppler::Document::SplashBackend);
//	m_tikzPdfDoc->setRenderBackend(Poppler::Document::ArthurBackend);
	m_tikzPdfDoc->setRenderHint(Poppler::Document::Antialiasing, true);
	m_tikzPdfDoc->setRenderHint(Poppler::Document::TextAntialiasing, true);
	const int numOfPages = m_tikzPdfDoc->numPages();

	const bool visible = (numOfPages > 1);
	if (m_pageSeparator)
		m_pageSeparator->setVisible(visible);
	m_previousPageAction->setVisible(visible);
	m_nextPageAction->setVisible(visible);

	if (m_currentPage >= numOfPages) // if the new tikz code has fewer tikzpictures than the previous one (this may happen if a new PGF file is opened in the same window), then we must reset m_currentPage
	{
		m_currentPage = 0;
		m_previousPageAction->setEnabled(false);
		m_nextPageAction->setEnabled(true);
	}

	showPdfPage();
}

/***************************************************************************/

QPixmap TikzPreview::pixmap() const
{
	return m_tikzPixmapItem->pixmap();
}

int TikzPreview::currentPage() const
{
	return m_currentPage;
}

int TikzPreview::numberOfPages() const
{
	return m_tikzPdfDoc->numPages();
}

/***************************************************************************/

void TikzPreview::centerInfoLabel()
{
	qreal posX;
	qreal posY;
	if (sceneRect().width() > viewport()->width())
		posX = horizontalScrollBar()->value() + (viewport()->width() - m_infoWidget->width()) * 0.5;
	else
		posX = (sceneRect().width() - m_infoWidget->width()) * 0.5;
	if (sceneRect().height() > viewport()->height())
		posY = verticalScrollBar()->value() + (viewport()->height() - m_infoWidget->height()) * 0.5;
	else
		posY = (sceneRect().height() - m_infoWidget->height()) * 0.5;

	m_infoWidget->move(posX, posY);
}

void TikzPreview::setInfoLabelText(const QString &message, PixmapVisibility pixmapVisibility)
{
	m_infoPixmapLabel->setVisible(pixmapVisibility == PixmapVisible);
	m_infoLabel->setText(message);
//	m_infoWidget->setVisible(false);
//	m_infoWidget->setVisible(true);
	if (m_infoProxyWidget->scene() != 0) // only remove if the widget is still attached to m_tikzScene
		m_tikzScene->removeItem(m_infoProxyWidget); // make sure that any previous messages are not visible anymore
	m_tikzScene->addItem(m_infoProxyWidget);
	centerInfoLabel(); // must be run here so that the label is always centered
	m_infoWidgetAdded = true;
}

void TikzPreview::showErrorMessage(const QString &message)
{
	setInfoLabelText(message, PixmapVisible);
}

void TikzPreview::setProcessRunning(bool isRunning)
{
	m_processRunning = isRunning;
	if (isRunning)
		setInfoLabelText(tr("Generating image", "tikz preview status"), PixmapNotVisible);
//	else
//		m_infoWidget->setVisible(false);
	else if (m_infoProxyWidget->scene() != 0) // only remove if the widget is still attached to m_tikzScene
		m_tikzScene->removeItem(m_infoProxyWidget);
}

/***************************************************************************/

void TikzPreview::setShowCoordinates(bool show)
{
	m_showCoordinates = show;
}

void TikzPreview::setCoordinatePrecision(int precision)
{
	m_precision = precision;
}

/***************************************************************************/

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

void TikzPreview::mouseMoveEvent(QMouseEvent *event)
{
	const int offset = 6 * m_currentPage;
	if (m_showCoordinates && m_tikzCoordinates.length() >= offset + 6)
	{
		const qreal unitX = m_tikzCoordinates.at(offset);
		const qreal unitY = m_tikzCoordinates.at(1 + offset);
		const qreal minX = m_tikzCoordinates.at(2 + offset);
		const qreal maxX = m_tikzCoordinates.at(3 + offset);
		const qreal minY = m_tikzCoordinates.at(4 + offset);
		const qreal maxY = m_tikzCoordinates.at(5 + offset);

		int precisionX = m_precision;
		int precisionY = m_precision;
		if (m_precision < 0)
		{
			qreal invUnitX = 1 / unitX;
			qreal invUnitY = 1 / unitY;
			for (precisionX = 0; invUnitX < 1; ++precisionX)
				invUnitX *= 10;
			for (precisionY = 0; invUnitY < 1; ++precisionY)
				invUnitY *= 10;
		}

		qreal cursorX = event->x() + horizontalScrollBar()->value() - qMax(qreal(0.0), viewport()->width() - m_tikzPixmapItem->boundingRect().width()) / 2;
		cursorX /= m_zoomFactor;
		qreal cursorY = event->y() + verticalScrollBar()->value() - qMax(qreal(0.0), viewport()->height() - m_tikzPixmapItem->boundingRect().height()) / 2;
		cursorY /= m_zoomFactor;
		const qreal coordX = cursorX + minX;
		const qreal coordY = maxY - cursorY;
		if (coordX >= minX && coordX <= maxX && coordY >= minY && coordY <= maxY)
			emit showMouseCoordinates(coordX / unitX, coordY / unitY, precisionX, precisionY);
	}
	QGraphicsView::mouseMoveEvent(event);
}

void TikzPreview::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::XButton1)
		showPreviousPage();
	else if (event->button() == Qt::XButton2)
		showNextPage();
	QGraphicsView::mousePressEvent(event);
}
