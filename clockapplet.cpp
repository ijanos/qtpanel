#include "clockapplet.h"

#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtGui/QGraphicsScene>
#include "textgraphicsitem.h"
#include "panelwindow.h"

ClockApplet::ClockApplet(PanelWindow* panelWindow)
	: Applet(panelWindow)
{
	m_timer = new QTimer();
	m_timer->setSingleShot(true);
	connect(m_timer, SIGNAL(timeout()), this, SLOT(updateContent()));
	m_textItem = new TextGraphicsItem(this);
	m_textItem->setColor(Qt::white);
	m_textItem->setFont(m_panelWindow->font());
}

ClockApplet::~ClockApplet()
{
	delete m_textItem;
	delete m_timer;
}

bool ClockApplet::init()
{
	updateContent();

	setInteractive(true);
	return true;
}

void ClockApplet::layoutChanged()
{
	m_textItem->setPos((m_size.width() - m_textItem->boundingRect().size().width())/2.0, m_panelWindow->textBaseLine());
}

void ClockApplet::updateContent()
{
	QDateTime dateTimeNow = QDateTime::currentDateTime();
	m_text = dateTimeNow.toString("yyyy-MM-dd hh:mm");
	m_textItem->setText(m_text);
	update();
	scheduleUpdate();
}

QSize ClockApplet::desiredSize()
{
	return QSize(180, -1);
}

void ClockApplet::scheduleUpdate()
{
	m_timer->setInterval(1000 - QDateTime::currentDateTime().time().msec());
	m_timer->start();
}

