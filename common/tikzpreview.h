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

#ifndef KTIKZ_TIKZPREVIEW_H
#define KTIKZ_TIKZPREVIEW_H

#include <QtGui/QGraphicsView>

class QImage;
class QLabel;
class QToolBar;

namespace Poppler
{
class Document;
}

class Action;
class ZoomAction;
class TikzPreviewRenderer;

class TikzPreview : public QGraphicsView
{
	Q_OBJECT

public:
	TikzPreview(QWidget *parent = 0);
	~TikzPreview();

	virtual QSize sizeHint() const;
	QList<QAction*> actions();
	QToolBar *toolBar();
	QPixmap pixmap() const;
	int currentPage() const;
	int numberOfPages() const;
	void emptyPreview();
	void setProcessRunning(bool isRunning);
	void setShowCoordinates(bool show);
	void setCoordinatePrecision(int precision);

public slots:
	void showPreview(const QImage &tikzImage, qreal zoomFactor = 1.0);
	void pixmapUpdated(Poppler::Document *tikzPdfDoc, const QList<qreal> &tikzCoordinates = QList<qreal>());
	void showErrorMessage(const QString &message);

signals:
	void showMouseCoordinates(qreal x, qreal y, int precisionX = 5, int precisionY = 5);
	void generatePreview(Poppler::Document *tikzPdfDoc, qreal zoomFactor, int currentPage);

protected:
	void contextMenuEvent(QContextMenuEvent *event);
	void paintEvent(QPaintEvent *event);
	void wheelEvent(QWheelEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);

private slots:
	void setZoomFactor(qreal zoomFactor);
	void zoomIn();
	void zoomOut();
	void showPreviousPage();
	void showNextPage();

private:
	enum PixmapVisibility
	{
		PixmapNotVisible = 0,
		PixmapVisible = 1
	};

	void createInformationLabel();
	void createActions();
	void showPdfPage();
	void centerInfoLabel();
	void setInfoLabelText(const QString &message, PixmapVisibility pixmapVisibility = PixmapNotVisible);

	QGraphicsScene *m_tikzScene;
	QGraphicsPixmapItem *m_tikzPixmapItem;
	TikzPreviewRenderer *m_tikzPreviewRenderer;
	bool m_processRunning;

	QAction *m_zoomInAction;
	QAction *m_zoomOutAction;
	ZoomAction *m_zoomToAction;
	QAction *m_pageSeparator;
	Action *m_previousPageAction;
	Action *m_nextPageAction;

	QFrame *m_infoWidget;
	QGraphicsItem *m_infoProxyWidget;
	QLabel *m_infoPixmapLabel;
	QLabel *m_infoLabel;
	bool m_infoWidgetAdded;

	Poppler::Document *m_tikzPdfDoc;
	int m_currentPage;
	qreal m_zoomFactor;
	qreal m_oldZoomFactor;
	bool m_hasZoomed;

	bool m_showCoordinates;
	QList<qreal> m_tikzCoordinates;
	int m_precision;
};

#endif
