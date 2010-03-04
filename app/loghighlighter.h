/***************************************************************************
 *   Copyright (C) 2007 by Florian Hackenberger                            *
 *   Copyright (C) 2007 by Glad Deschrijver                                *
 *   florian@hackenberger.at                                               *
 *   glad.deschrijver@gmail.com                                            *
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

#ifndef LOGHIGHLIGHTER_H
#define LOGHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>


/** A simple, incomplete highlighter for LaTeX .log files
 * @author Florian Hackenberger <florian@hackenberger.at>
 */
class LogHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	LogHighlighter(QTextDocument *parent = 0);
	virtual ~LogHighlighter();

protected:
	/** Implements QSyntaxHighlighter::highlightBlock()
	 * @see QSyntaxHighlighter::highlightBlock()
	 */
	void highlightBlock(const QString &text);

private:
	struct HighlightingRule
	{
		QRegExp pattern; /// The pattern to match for formatting
		QTextCharFormat format; /// The style of the formatting
	};
	/// All highlighting rules with their formatting for easy iteration
	QVector<HighlightingRule> m_highlightingRules;
	/// The start of the statistics output by LaTeX
	QString m_statisticsStartExpression;
	/// The highlighting format for LaTeX statistics
	QTextCharFormat m_statisticsFormat;
};

#endif