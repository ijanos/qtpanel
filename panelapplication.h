#ifndef PANELAPPLICATION_H
#define PANELAPPLICATION_H

#include <QtGui/QApplication>
#include "panelwindow.h"

class X11Support;

class PanelApplication: public QApplication
{
	Q_OBJECT
public:
	PanelApplication(int& argc, char** argv);
	~PanelApplication();

	static PanelApplication* instance()
	{
		return m_instance;
	}

	bool x11EventFilter(XEvent* event);

private:
	static PanelApplication* m_instance;
	X11Support* m_x11support;
	PanelWindow* m_panelWindow;
};

#endif
