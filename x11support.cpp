#include "x11support.h"

#include <QtGui/QApplication>
#include <QtGui/QX11Info>
#include <QtGui/QImage>

// Keep all the X11 stuff with scary defines below normal headers.
#include <X11/Xlib.h>
#include <X11/Xatom.h>

X11Support* X11Support::m_instance = NULL;

X11Support::X11Support()
{
	m_instance = this;
}

X11Support::~X11Support()
{
	m_instance = NULL;
}

unsigned long X11Support::rootWindow()
{
	return QX11Info::appRootWindow();
}

unsigned long X11Support::atom(const QString& name)
{
	if(!m_cachedAtoms.contains(name))
		m_cachedAtoms[name] = XInternAtom(QX11Info::display(), name.toLatin1().data(), False);
	return m_cachedAtoms[name];
}

void X11Support::removeWindowProperty(unsigned long window, const QString& name)
{
	XDeleteProperty(QX11Info::display(), window, atom(name));
}

void X11Support::setWindowPropertyCardinalArray(unsigned long window, const QString& name, const QVector<unsigned long>& values)
{
	XChangeProperty(QX11Info::display(), window, atom(name), XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(values.data()), values.size());
}

template<class T>
static bool getWindowPropertyHelper(unsigned long window, unsigned long atom, unsigned long type, int& numItems, T*& data)
{
	Atom retType;
	int retFormat;
	unsigned long numItemsTemp;
	unsigned long bytesLeft;
	if(XGetWindowProperty(QX11Info::display(), window, atom, 0, 0x7FFFFFFF, False, type, &retType, &retFormat, &numItemsTemp, &bytesLeft, reinterpret_cast<unsigned char**>(&data)) != Success)
		return false;
	numItems = numItemsTemp;
	return true;
}

QVector<unsigned long> X11Support::getWindowPropertyWindowsArray(unsigned long window, const QString& name)
{
	int numItems;
	unsigned long* data;
	QVector<unsigned long> values;
	if(!getWindowPropertyHelper(window, atom(name), XA_WINDOW, numItems, data))
		return values;
	for(int i = 0; i < numItems; i++)
		values.append(data[i]);
	XFree(data);
	return values;
}

QVector<unsigned long> X11Support::getWindowPropertyAtomsArray(unsigned long window, const QString& name)
{
	int numItems;
	unsigned long* data;
	QVector<unsigned long> values;
	if(!getWindowPropertyHelper(window, atom(name), XA_ATOM, numItems, data))
		return values;
	for(int i = 0; i < numItems; i++)
		values.append(data[i]);
	XFree(data);
	return values;
}

QString X11Support::getWindowPropertyUTF8String(unsigned long window, const QString& name)
{
	int numItems;
	char* data;
	QString value;
	if(!getWindowPropertyHelper(window, atom(name), atom("UTF8_STRING"), numItems, data))
		return value;
	value = QString::fromUtf8(data);
	XFree(data);
	return value;
}

QString X11Support::getWindowPropertyLatin1String(unsigned long window, const QString& name)
{
	int numItems;
	char* data;
	QString value;
	if(!getWindowPropertyHelper(window, atom(name), XA_STRING, numItems, data))
		return value;
	value = QString::fromLatin1(data);
	XFree(data);
	return value;
}

QString X11Support::getWindowName(unsigned long window)
{
	QString result = getWindowPropertyUTF8String(window, "_NET_WM_VISIBLE_NAME");
	if(result.isEmpty())
		result = getWindowPropertyUTF8String(window, "_NET_WM_NAME");
	if(result.isEmpty())
		result = getWindowPropertyLatin1String(window, "WM_NAME");
	if(result.isEmpty())
		result = "<Unknown>";
	return result;
}

QIcon X11Support::getWindowIcon(unsigned long window)
{
	int numItems;
	unsigned long* rawData;
	QIcon icon;
	if(!getWindowPropertyHelper(window, atom("_NET_WM_ICON"), XA_CARDINAL, numItems, rawData))
		return icon;
	unsigned long* data = rawData;
	while(numItems > 0)
	{
		int width = static_cast<int>(data[0]);
		int height = static_cast<int>(data[1]);
		data += 2;
		numItems -= 2;
		QImage image(width, height, QImage::Format_ARGB32);
		for(int i = 0; i < height; i++)
		{
			for(int k = 0; k < width; k++)
			{
				image.setPixel(k, i, static_cast<unsigned int>(data[i*width + k]));
			}
		}
		data += width*height;
		numItems -= width*height;
		icon.addPixmap(QPixmap::fromImage(image));
	}
	XFree(rawData);
	return icon;
}

void X11Support::registerForWindowPropertyChanges(unsigned long window)
{
	XSelectInput(QX11Info::display(), window, PropertyChangeMask);
}
