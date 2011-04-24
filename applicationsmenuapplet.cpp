#include "applicationsmenuapplet.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QProcess>
#include <QtGui/QMenu>
#include <QtGui/QStyle>
#include <QtGui/QPixmap>
#include <QtGui/QGraphicsScene>
#include "textgraphicsitem.h"
#include "panelwindow.h"

int ApplicationsMenuStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
	if(metric == QStyle::PM_SmallIconSize)
		return 32;
	else
		return QPlastiqueStyle::pixelMetric(metric, option, widget);
}

bool DesktopFile::init(const QString& fileName)
{
	QFile file(fileName);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QTextStream in(&file);
	while(!in.atEnd())
	{
		QString line = in.readLine();
		if(line[0] == '[' || line[0] == '#')
			continue;
		QStringList list = line.split('=');
		if(list.size() < 2)
			continue;
		QString key = list[0];
		QString value = list[1];
		if(key == "NoDisplay" && value == "true")
			return false;
		if(key == "Name")
			m_name = value;
		if(key == "Exec")
			m_exec = value;
		if(key == "Icon")
			m_icon = value;
		if(key == "Categories")
			m_categories = value.split(";", QString::SkipEmptyParts);
	}
	return true;
}

SubMenu::SubMenu(QMenu* parent, const QString& title, const QString& category, const QString& icon)
{
	m_menu = new QMenu(parent); // Will be deleted automatically.
	m_menu->setStyle(parent->style());
	m_menu->setFont(parent->font());
	m_menu->setTitle(title);
	m_menu->setIcon(QIcon::fromTheme(icon));
	m_menu->menuAction()->setIconVisibleInMenu(true);
	m_category = category;
}

ApplicationsMenuApplet::ApplicationsMenuApplet(PanelWindow* panelWindow)
	: Applet(panelWindow), m_menuOpened(false)
{
	m_menu = new QMenu();
	m_menu->setStyle(&m_style);
	m_menu->setFont(m_panelWindow->font());
	m_menu->setStyleSheet("QMenu { background-color: black; } QMenu::item { height: 36px; background-color: transparent; color: white; padding-left: 38px; padding-right: 20px; padding-top: 2px; padding-bottom: 2px; } QMenu::item::selected { background-color: #606060; border-color: gray; } QMenu::icon { left: 2px; }");
	m_subMenus.append(SubMenu(m_menu, "Accessories", "Utility", "applications-accessories"));
	m_subMenus.append(SubMenu(m_menu, "Development", "Development", "applications-development"));
	m_subMenus.append(SubMenu(m_menu, "Education", "Education", "applications-science"));
	m_subMenus.append(SubMenu(m_menu, "Office", "Office", "applications-office"));
	m_subMenus.append(SubMenu(m_menu, "Graphics", "Graphics", "applications-graphics"));
	m_subMenus.append(SubMenu(m_menu, "Multimedia", "AudioVideo", "applications-multimedia"));
	m_subMenus.append(SubMenu(m_menu, "Games", "Game", "applications-games"));
	m_subMenus.append(SubMenu(m_menu, "Network", "Network", "applications-internet"));
	m_subMenus.append(SubMenu(m_menu, "System", "System", "preferences-system"));
	m_subMenus.append(SubMenu(m_menu, "Settings", "Settings", "preferences-desktop"));
	m_subMenus.append(SubMenu(m_menu, "Other", "Other", "applications-other"));

	m_textItem = new TextGraphicsItem(this);
	m_textItem->setColor(Qt::white);
	m_textItem->setFont(m_panelWindow->font());
	m_textItem->setText("Applications");
}

ApplicationsMenuApplet::~ApplicationsMenuApplet()
{
	delete m_textItem;
	delete m_menu;
}

bool ApplicationsMenuApplet::init()
{
	updateDesktopFiles();

	setInteractive(true);
	return true;
}

QSize ApplicationsMenuApplet::desiredSize()
{
	return QSize(m_textItem->boundingRect().size().width() + 16, m_textItem->boundingRect().size().height());
}

void ApplicationsMenuApplet::clicked()
{
	m_menuOpened = true;
	animateHighlight();

	// Re-init submenus here to keep order stable.
	m_menu->clear();
	for(int i = 0; i < m_subMenus.size(); i++)
	{
		if(m_subMenus[i].menu()->actions().size() > 0)
			m_menu->addMenu(m_subMenus[i].menu());
	}

	m_menu->move(localToScreen(QPoint(0, m_size.height())));
	m_menu->exec();

	m_menuOpened = false;
	animateHighlight();
}

void ApplicationsMenuApplet::layoutChanged()
{
	m_textItem->setPos(8, m_panelWindow->textBaseLine());
}

bool ApplicationsMenuApplet::isHighlighted()
{
	return m_menuOpened || Applet::isHighlighted();
}

void ApplicationsMenuApplet::actionTriggered()
{
	QString exec = static_cast<QAction*>(sender())->data().toString();

	// Handle special arguments.
	for(;;)
	{
		int argPos = exec.indexOf('%');
		if(argPos == -1)
			break;
		// For now, just remove them.
		int spacePos = exec.indexOf(' ', argPos);
		if(spacePos == -1)
			exec.resize(argPos);
		else
			exec.remove(argPos, spacePos - argPos);
	}

	exec = exec.trimmed();
	QStringList args = exec.split(' ');
	QString process = args[0];
	args.removeAt(0);
	QProcess::startDetached(process, args, getenv("HOME"));
}

void ApplicationsMenuApplet::updateDesktopFiles()
{
	m_desktopFiles.clear();
	gatherDesktopFiles("/usr/share/applications");
}

void ApplicationsMenuApplet::gatherDesktopFiles(const QString& path)
{
	QDir dir(path);
	QStringList list = dir.entryList(QStringList("*.desktop"));
	foreach(const QString& fileName, list)
	{
		desktopFileAdded(dir.path() + '/' + fileName);
	}
	// Traverse subdirectories recursively.
	// Apparently, KDE keeps all it's programs in a subdirectory.
	QStringList subDirList = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	foreach(const QString& subDirName, subDirList)
	{
		gatherDesktopFiles(path + '/' + subDirName);
	}
}

void ApplicationsMenuApplet::desktopFileAdded(const QString& fileName)
{
	DesktopFile desktopFile;
	if(desktopFile.init(fileName))
	{
		m_desktopFiles[fileName] = desktopFile;

		QAction* action = new QAction(m_menu); // Will be deleted automatically.
		action->setText(desktopFile.name());
		QIcon icon = QIcon::fromTheme(desktopFile.icon());
		if(icon.isNull())
		{
			if(desktopFile.icon().contains('/'))
				icon = QIcon(desktopFile.icon());
			else
				icon = QIcon("/usr/share/pixmaps/" + desktopFile.icon());
		}
		int iconSize = m_menu->style()->pixelMetric(QStyle::PM_SmallIconSize);
		if(!icon.availableSizes().empty())
		{
			if(!icon.availableSizes().contains(QSize(iconSize, iconSize)))
			{
				QPixmap pixmap = icon.pixmap(256); // Any big size here is fine (at least for now).
				QPixmap scaledPixmap = pixmap.scaled(QSize(iconSize, iconSize), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				icon = QIcon(scaledPixmap);
			}
		}
		action->setIcon(icon);
		action->setIconVisibleInMenu(true);
		action->setData(desktopFile.exec());
		connect(action, SIGNAL(triggered()), this, SLOT(actionTriggered()));

		// Add to relevant menu.
		int subMenuIndex = m_subMenus.size() - 1; // By default put it in "Other".
		for(int i = 0; i < m_subMenus.size() - 1; i++) // Without "Other".
		{
			if(desktopFile.categories().contains(m_subMenus[i].category()))
			{
				subMenuIndex = i;
				break;
			}
		}
		m_subMenus[subMenuIndex].menu()->addAction(action);
		desktopFile.setAction(action);
	}
}
