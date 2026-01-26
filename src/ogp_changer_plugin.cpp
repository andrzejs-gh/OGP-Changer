#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>
#include <KPluginFactory>
#include <QWidget>
#include <QIcon>
#include <QAction>
#include <QProcess>
#include <QUrl>
#include <QByteArray>
#include <QDir>
#include <QString>

class ChangeOwnerGroupPermissions : public KAbstractFileItemActionPlugin
{
	Q_OBJECT
	
	QProcess* proc;
	QString home_dir;
	
	public:
    explicit ChangeOwnerGroupPermissions(QObject* parent) 
    : KAbstractFileItemActionPlugin(parent)
    {
		home_dir = QDir::homePath();
		proc = new QProcess(this);
    }
    
    QList<QAction*> actions(const KFileItemListProperties& fileItemInfos, QWidget* parent) override
    {
        QList<QAction*> list;
        QList<QUrl> urls = fileItemInfos.urlList();

        QAction* action = new QAction("Change owner / group / permissions", parent);
        action->setIcon(QIcon::fromTheme("view-process-users"));
        connect(action, &QAction::triggered, this,
        [urls, this]()
        {
			QByteArray data;
            for (const QUrl& u : urls)
            {
                data += u.path().toUtf8();
			    data.append('\0');
			}
			
            proc->setProgram(home_dir + "/.local/bin/ogp_changer");
            proc->start();
			proc->write(data);
            proc->closeWriteChannel();
        });

        list << action;
        return list;
    }
};

K_PLUGIN_FACTORY_WITH_JSON(ChangeOwnerGroupPermissionsFactory,
                           "ogp_changer_plugin.json",
                           registerPlugin<ChangeOwnerGroupPermissions>();)

#include "ogp_changer_plugin.moc"
