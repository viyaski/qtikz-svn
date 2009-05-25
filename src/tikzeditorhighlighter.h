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

#ifndef TIKZHIGHLIGHTER_H
#define TIKZHIGHLIGHTER_H

#include <QSyntaxHighlighter>

class TikzCommandInserter;

struct HighlightingRule
{
	QString type; /// The name of the formatting type
	QRegExp pattern; /// The pattern to match for formatting
};

/** A simple highlighter for the TikZ graphics programming language
 * @author Florian Hackenberger <florian@hackenberger.at>
 */
class TikzHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	TikzHighlighter(TikzCommandInserter *commandInserter = 0, QTextDocument *parent = 0);
	virtual ~TikzHighlighter();

	QMap<QString, QTextCharFormat> getDefaultHighlightFormats();
	QStringList getTranslatedHighlightTypeNames();
	QStringList getHighlightTypeNames();
	QMap<QString, QTextCharFormat> getTextCharFormats();
	void setTextCharFormats(const QMap<QString, QTextCharFormat> &formatList);

protected:
	/** Implements QSyntaxHighlighter::highlightBlock()
	 * @see QSyntaxHighlighter::highlightBlock()
	 */
	void highlightBlock(const QString &text);

private:
	/// All highlighting rules with their formatting for easy iteration
	QVector<HighlightingRule> m_highlightingRules;

	TikzCommandInserter *m_commandInserter;
	QMap<QString, QTextCharFormat> m_formatList;
	QStringList m_highlightTypeNames;
};

#endif
