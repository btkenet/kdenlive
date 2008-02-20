/***************************************************************************
                          parameterplotter.h  -  description
                             -------------------
    begin                : Feb 15 2008
    copyright            : (C) 2008 by Marco Gittler
    email                : g.marco@freenet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <KPlotWidget>

class ParameterPlotter : public KPlotWidget {
	Q_OBJECT
	public:
		ParameterPlotter (QWidget *parent=0);
		virtual ~ParameterPlotter(){}
		void setMoveX(bool);
		void setMoveY(bool);
		void setMoveTimeLine(bool);
		void setNewPoints(bool);
		bool isMoveX();
		bool isMoveY();
		bool isMoveTimeline();
		bool isNewPoints();
	private:
		KPlotPoint* movepoint;
		bool m_moveX,m_moveY,m_moveTimeline,m_newPoints;
		QPoint oldmousepoint;
		int maxx,maxy;
		QStringList parameterNameList;
		void createParametersNew();
		QList<KPlotObject*> plotobjects;
		QList<QColor> colors;
	protected:
		void mouseMoveEvent ( QMouseEvent * event );
		void mousePressEvent ( QMouseEvent * event );
	public slots:
		void setPointLists(const QList< QPair<QString, QMap<int,QVariant> > >&,int,int);
	signals:
		void parameterChanged(QList< QPair<QString, QMap<int,QVariant> > > );
	
};
