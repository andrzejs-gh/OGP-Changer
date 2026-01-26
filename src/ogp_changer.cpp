#include <iostream>
#include <cstdio>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QObject>
#include <QMetaObject>
#include <Qt>
#include <QStringList>
#include <QProcess>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QByteArray> 
#include <QTemporaryFile>
#include <QStandardPaths>

class ErrorDialog : public QDialog
{
	QStringList error_string_list;
	QVBoxLayout* main_layout;
	QLabel* label;
	QPlainTextEdit* error_box;
	QPushButton* ok_button;
	
	public:
	ErrorDialog(const QStringList& error_string_list, QWidget* parent = nullptr) 
	: QDialog(parent), error_string_list(error_string_list)
	{
		setWindowTitle("Errors");
		main_layout = new QVBoxLayout(this);
		
		label = new QLabel("The following errors occurred during the operation:", this);
		error_box = new QPlainTextEdit(this);
		error_box->setReadOnly(true);
		error_box->setPlainText(error_string_list.join(""));
		ok_button = new QPushButton("Ok", this);
		
		main_layout->addWidget(label);
		main_layout->addWidget(error_box);
		main_layout->addWidget(ok_button);
		
		connect(ok_button, &QPushButton::clicked, this, &QDialog::accept);
		
		adjustSize();
		setFixedSize(2*height(), height());
	}
};

class GlobalOverlay : public QWidget
{
	public:
	GlobalOverlay(QWidget* parent) : QWidget(parent)
	{
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        setWindowFlags(Qt::FramelessWindowHint);
        setStyleSheet("background: rgba(0, 0, 0, 100);");
        hide();
	}
};

class MainWindow : public QDialog
{
	bool invalid_owner, invalid_group, run_as_root;
	struct stat file_stat;
	GlobalOverlay *overlay;
	QByteArray stdin_buffer;
	QString temp_file_path;
	QStringList args, errors;
	QProcess *proc;
	QVBoxLayout *main_layout;
    QGridLayout *grid_layout;
    QHBoxLayout *buttons_layout, *run_as_root_chbx_layout, *apply_rec_chbx_layout;
    QCheckBox *change_owner_chbx, *change_group_chbx; 
    QComboBox *owner_combo, *group_combo, *others_combo;
    QCheckBox *run_as_root_chbx, *apply_rec_chbx, *change_permissions_chbx;
    QCheckBox *ow_r_chbx, *ow_w_chbx, *ow_x_chbx, *setuid_chbx; 
    QCheckBox *gr_r_chbx, *gr_w_chbx, *gr_x_chbx, *setgid_chbx;
    QCheckBox *ot_r_chbx, *ot_w_chbx, *ot_x_chbx, *sticky_chbx;
    QPushButton *ok_button, *cancel_button;
    
	public:
    MainWindow(const QStringList& args, struct stat& file_stat, QByteArray& stdin_buffer, 
				QWidget* parent = nullptr) 
    : QDialog(parent), args(args), file_stat(file_stat), stdin_buffer(stdin_buffer)
    {
		setWindowTitle("Change owner, group or permissions");
		invalid_owner = false; 
		invalid_group = false;
		run_as_root = false;
		
		overlay = new GlobalOverlay(this);
		
		main_layout = new QVBoxLayout(this);
		run_as_root_chbx_layout = new QHBoxLayout();
		main_layout->addLayout(run_as_root_chbx_layout);
		apply_rec_chbx_layout = new QHBoxLayout();
		main_layout->addLayout(apply_rec_chbx_layout);
		main_layout->addSpacing(20);
		grid_layout = new QGridLayout();
		main_layout->addLayout(grid_layout);
		buttons_layout = new QHBoxLayout();
		main_layout->addSpacing(20);
		main_layout->addLayout(buttons_layout);
		
		run_as_root_chbx = new QCheckBox("Run with root privileges", this);
		apply_rec_chbx = new QCheckBox("Apply changes recursively\n(symlinks below top-level paths are ignored)", this);
		
		change_permissions_chbx = new QCheckBox("Change permissions", this);
		change_permissions_chbx->setChecked(true);
		connect(change_permissions_chbx, &QCheckBox::toggled, 
		        this, &MainWindow::on_checkboxes_change);
		
		change_owner_chbx = new QCheckBox("Change owner: ", this);
		connect(change_owner_chbx, &QCheckBox::toggled, 
		        this, &MainWindow::on_checkboxes_change);
		change_group_chbx = new QCheckBox("Change group: ", this);
		connect(change_group_chbx, &QCheckBox::toggled, 
		        this, &MainWindow::on_checkboxes_change);
		
		// === owner combo ========================
		owner_combo = new QComboBox(this);
		
		// adding users to owner combo:
		struct passwd* pw;
		setpwent();
		while ((pw = getpwent()) != nullptr)
			owner_combo->addItem(pw->pw_name);
		endpwent();

		owner_combo->setEditable(true);
		owner_combo->setEnabled(false);
		connect(owner_combo->lineEdit(), &QLineEdit::textChanged,
				this, &MainWindow::on_combo_text_change);
		// ========================================
		
		// === group combo ========================
		group_combo = new QComboBox(this);
		
		// adding groups to group combo
		struct group* gr;
		setgrent();
		while ((gr = getgrent()) != nullptr)
			group_combo->addItem(gr->gr_name);
		endgrent();
		
		group_combo->setEditable(true);
		group_combo->setEnabled(false);
		connect(group_combo->lineEdit(), &QLineEdit::textChanged,
				this, &MainWindow::on_combo_text_change);
		// ========================================
		
		others_combo = new QComboBox(this);
		others_combo->addItem("others");
		others_combo->setEnabled(false);
		
		mode_t mode = file_stat.st_mode;
		
		ow_r_chbx = new QCheckBox("r", this);
		ow_w_chbx = new QCheckBox("w", this);
		ow_x_chbx = new QCheckBox("x", this);
		setuid_chbx = new QCheckBox("Set UID", this);
		ow_r_chbx->setChecked(mode & S_IRUSR);
		ow_w_chbx->setChecked(mode & S_IWUSR);
		ow_x_chbx->setChecked(mode & S_IXUSR);
		setuid_chbx->setChecked(mode & S_ISUID);
		
		gr_r_chbx = new QCheckBox("r", this);
		gr_w_chbx = new QCheckBox("w", this);
		gr_x_chbx = new QCheckBox("x", this);
		setgid_chbx = new QCheckBox("Set GID", this);
		gr_r_chbx->setChecked(mode & S_IRGRP);
		gr_w_chbx->setChecked(mode & S_IWGRP);
		gr_x_chbx->setChecked(mode & S_IXGRP);
		setgid_chbx->setChecked(mode & S_ISGID);
		
		ot_r_chbx = new QCheckBox("r", this);
		ot_w_chbx = new QCheckBox("w", this);
		ot_x_chbx = new QCheckBox("x", this);
		sticky_chbx = new QCheckBox("Sticky", this);
		ot_r_chbx->setChecked(mode & S_IROTH);
		ot_w_chbx->setChecked(mode & S_IWOTH);
		ot_x_chbx->setChecked(mode & S_IXOTH);
		sticky_chbx->setChecked(mode & S_ISVTX);
		
		ok_button = new QPushButton("Ok", this);
		cancel_button = new QPushButton("Cancel", this);
		connect(ok_button, &QPushButton::clicked, this, &MainWindow::on_ok_clicked);
		connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
		
		run_as_root_chbx_layout->addWidget(run_as_root_chbx);
		apply_rec_chbx_layout->addWidget(apply_rec_chbx);
		
		grid_layout->addWidget(change_permissions_chbx, 0, 2, 1, 5);
		
		grid_layout->addWidget(change_owner_chbx, 1, 0);
		grid_layout->addWidget(owner_combo, 1, 1);
		grid_layout->addWidget(ow_r_chbx, 1, 2);
		grid_layout->addWidget(ow_w_chbx, 1, 3);
		grid_layout->addWidget(ow_x_chbx, 1, 4);
		grid_layout->addWidget(new QLabel("  ", this), 1, 5);
		grid_layout->addWidget(setuid_chbx, 1, 6);
		
		grid_layout->addWidget(change_group_chbx, 2, 0);
		grid_layout->addWidget(group_combo, 2, 1);
		grid_layout->addWidget(gr_r_chbx, 2, 2);
		grid_layout->addWidget(gr_w_chbx, 2, 3);
		grid_layout->addWidget(gr_x_chbx, 2, 4);
		grid_layout->addWidget(new QLabel("  ", this), 2, 5);
		grid_layout->addWidget(setgid_chbx, 2, 6);
		
		grid_layout->addWidget(others_combo, 3, 1);
		grid_layout->addWidget(ot_r_chbx, 3, 2);
		grid_layout->addWidget(ot_w_chbx, 3, 3);
		grid_layout->addWidget(ot_x_chbx, 3, 4);
		grid_layout->addWidget(new QLabel("  ", this), 3, 5);
		grid_layout->addWidget(sticky_chbx, 3, 6);

		buttons_layout->addWidget(ok_button);
		buttons_layout->addWidget(cancel_button);
		
		select_owner_and_group();
		change_permissions_chbx->setChecked(false);
		proc = new QProcess(this);
		connect(proc, &QProcess::finished, this, &MainWindow::on_cogp_finished);
		connect(proc, &QProcess::readyReadStandardError, this, &MainWindow::append_errors);
		connect(proc, &QProcess::errorOccurred, this, &MainWindow::on_proc_start_error);
		
		adjustSize();
		setFixedSize(width(), height());
	}
	
	void select_owner_and_group()
	{
		// selecting owner name
		uid_t uid = file_stat.st_uid;
		struct passwd* pw = getpwuid(uid);
		QString owner = pw ? QString(pw->pw_name) : QString::number(uid);
		owner_combo->setCurrentText(owner);
		
		// selecting group name
		gid_t gid = file_stat.st_gid;
		struct group* gr = getgrgid(gid);
		QString group = gr ? QString(gr->gr_name) : QString::number(gid);
		group_combo->setCurrentText(group);
	}
	
	void on_checkboxes_change()
	{
		QObject* obj = sender();
		if (obj == change_owner_chbx)
		{
			if (change_owner_chbx->isChecked()) 
				owner_combo->setEnabled(true);
			else
				owner_combo->setEnabled(false);
		}
		else if (obj == change_group_chbx)
		{
			if (change_group_chbx->isChecked()) 
				group_combo->setEnabled(true);
			else
				group_combo->setEnabled(false);
		}
		else // obj == change_permissions_chbx
		{
			bool state;
			if (change_permissions_chbx->isChecked())
				state = true;
			else
				state = false;

			ow_r_chbx->setEnabled(state);
			ow_w_chbx->setEnabled(state);
			ow_x_chbx->setEnabled(state);
			setuid_chbx->setEnabled(state);
			
			gr_r_chbx->setEnabled(state);
			gr_w_chbx->setEnabled(state);
			gr_x_chbx->setEnabled(state);
			setgid_chbx->setEnabled(state);
			
			ot_r_chbx->setEnabled(state);
			ot_w_chbx->setEnabled(state);
			ot_x_chbx->setEnabled(state);
			sticky_chbx->setEnabled(state);
		}
	}

	void on_combo_text_change()
	{
		QObject* obj = sender();
		if (obj == owner_combo->lineEdit()) 
			update_combo_color_n_state(owner_combo);
		else
			update_combo_color_n_state(group_combo);
	}
	
	void update_combo_color_n_state(QComboBox* combo)
	{
		QMetaObject::invokeMethod(combo, [this, combo]() {
			QString text = combo->currentText();
			QString color;
			
			if (combo->findText(text, Qt::MatchCaseSensitive) == -1)
			{
				color = "color: red;";
				if (combo == owner_combo)
					invalid_owner = true;
				else // combo == group_combo
					invalid_group = true;
			}
			else
			{
				color = "";
				if (combo == owner_combo)
					invalid_owner = false;
				else // combo == group_combo
					invalid_group = false;
			}
			combo->setStyleSheet(color);
			
		}, Qt::QueuedConnection);
	}
	
	void set_buttons_enabled(bool state)
	{
		ok_button->setEnabled(state);
		cancel_button->setEnabled(state);
	}
	
	void on_ok_clicked()
	{
		if (invalid_owner && owner_combo->isEnabled())
		{
			QMessageBox::warning(this, " ", "Invalid owner name.");
			return;
		}
		else if (invalid_group && group_combo->isEnabled())
		{
			QMessageBox::warning(this, " ", "Invalid group name.");
			return;
		}
		else if (!owner_combo->isEnabled() && !group_combo->isEnabled() && !change_permissions_chbx->isChecked())
		{
			QMessageBox::information(this, " ", "Nothing to change.");
			return;
		}
		
		run_as_root = false;
		set_buttons_enabled(false);
		overlay->setGeometry(this->rect());
		overlay->raise();
		overlay->show();
		
		QString cwd = QCoreApplication::applicationDirPath();
		QStringList pkexec_args;
		pkexec_args << cwd + "/cogp";
		if (apply_rec_chbx->isChecked())
			pkexec_args << "-r";
		pkexec_args << get_owner_group_permissions(); // owner, group, permissions
		if (stdin_buffer.isEmpty())
		{
			pkexec_args << args.mid(1); // paths (without the program path)
			temp_file_path = "";
		}
		else // if paths were passed via stdin
		{
			QString temp_directory;
			if (is_writable("/dev/shm"))
			    temp_directory = "/dev/shm";
			else if (is_writable("/tmp"))
				temp_directory = "/tmp";
			else
			    temp_directory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
			
			QTemporaryFile temp_file(temp_directory + "/paths_for_cogp_XXXXXX");
			temp_file.setAutoRemove(false);
			
			if (!temp_file.open())
			{
				QMessageBox::critical(this, "Temp file failure", 
									  "Temp file in: " 
									  + temp_directory + " could not be created.");
				overlay->hide();
				set_buttons_enabled(true);
				return;
			}
			temp_file.write(stdin_buffer.data(), stdin_buffer.size());
			temp_file.flush();
			temp_file_path = temp_file.fileName();
			pkexec_args << "--list" << temp_file_path;
		}
		
		if (run_as_root_chbx->isChecked())
		{
			run_as_root = true;
			proc->start("pkexec", pkexec_args);
		}
		else
			proc->start(cwd + "/cogp", pkexec_args.mid(1));
	}
	
	QStringList get_owner_group_permissions()
	{
		QString owner, group, permissions;
		// if cogp receives "/" as arg it will leave owner/group/permissions unchanged
		
		if (owner_combo->isEnabled())
			owner = owner_combo->currentText();
		else
			owner = "/";
		
		if (group_combo->isEnabled())
			group = group_combo->currentText();
		else
			group = "/";
			
		if (change_permissions_chbx->isChecked())
		{
			int special_bit = 0;
			int owner_bit = 0;
			int group_bit = 0;
			int others_bit = 0;

			// special bit
			if (setuid_chbx->isChecked()) special_bit += 4;
			if (setgid_chbx->isChecked()) special_bit += 2;
			if (sticky_chbx->isChecked()) special_bit += 1;

			// owner bit
			if (ow_r_chbx->isChecked()) owner_bit += 4;
			if (ow_w_chbx->isChecked()) owner_bit += 2;
			if (ow_x_chbx->isChecked()) owner_bit += 1;

			// group bit
			if (gr_r_chbx->isChecked()) group_bit += 4;
			if (gr_w_chbx->isChecked()) group_bit += 2;
			if (gr_x_chbx->isChecked()) group_bit += 1;

			//ohers_bit
			if (ot_r_chbx->isChecked()) others_bit += 4;
			if (ot_w_chbx->isChecked()) others_bit += 2;
			if (ot_x_chbx->isChecked()) others_bit += 1;

			permissions = QString::number(special_bit) +
						  QString::number(owner_bit) +
						  QString::number(group_bit) +
						  QString::number(others_bit);
		}
		else
			permissions = "/";
		
		QStringList cogp_args = {owner, group, permissions};
		
		return cogp_args;
	}
	
	bool is_writable(const QString& directory)
	{
		QTemporaryFile test_file(directory + "/test_file_XXXXXX");
	    return test_file.open();
	}
	
	void append_errors()
	{
		QByteArray data = proc->readAllStandardError();
		QString text = QString::fromUtf8(data);
		errors.append(text.replace("\x1E", "\n")); 
		// replacing \x1E with \n for cleaner display in ErrorDialog
	}
	
	void on_proc_start_error(QProcess::ProcessError error)
	{
		if (!temp_file_path.isEmpty()) // if temp file has been created delete it
			std::remove(temp_file_path.toUtf8().constData());
		
		if (error == QProcess::FailedToStart)
		{
			if (run_as_root)
			{
				QMessageBox::critical(this, "Error",
									  "pkexec could not be started. Is it installed?");
			}
			else
			{
				QMessageBox::critical(this, "Error",
								      "cogp could not be started. Is the binary installed in the application's directory?");
			}
		}
		else
		{
			QMessageBox::critical(this, "Error",
								  "An error occured. Please try again.");
		}
		overlay->hide();
		set_buttons_enabled(true);
	}
	
	void on_cogp_finished(int exit_code, QProcess::ExitStatus status)
	{
		if (!temp_file_path.isEmpty()) // if temp file has been created delete it
			std::remove(temp_file_path.toUtf8().constData());
		
		if (status == QProcess::NormalExit && exit_code == 0) 
		{
			if (!errors.isEmpty())
			{
				ErrorDialog er(errors, this);
				er.exec();
			}
			else
			{
				QMessageBox::information(this, " ", "Done.");
				this->accept();   // close main window
			}
		} 
		else 
		{
			QMessageBox::warning(this,
								 "Canceled or Failed",
								 "Operation was canceled or failed.");
		}
		errors.clear(); // clearing errors so they're not displayed on the next try
		overlay->hide();
		set_buttons_enabled(true);
	}
};

int main(int argc, char *argv[])
{
	struct stat file_stat;
	QByteArray stdin_buffer;
	QApplication app(argc, argv);
	
	if (argc == 1) // if no cmd line args passed attempt to read from stdin
	{
		stdin_buffer.reserve(65536);
		char chunk[65536]; // read in 64 KB chunks
		ssize_t read_bytes;
		while (true) 
		{
			read_bytes = read(STDIN_FILENO, chunk, sizeof(chunk));
			if (read_bytes <= 0)
			{
				if (read_bytes == 0) // EOF reached
					break;
				else // error
				{
					QMessageBox::critical(nullptr, "Critical error", "Error while reading stdin.");
					return 1;
				}
			}
			stdin_buffer.append(chunk, read_bytes);
		}
		if (stdin_buffer.isEmpty())
		{
			QMessageBox::information(nullptr, "Empty stdin_buffer", "No paths have been passed.");
			return 0;
		}
		// stating the first path:
		const char* first_path = stdin_buffer.constData();
		if (stat(first_path, &file_stat) != 0)
		{
			QMessageBox::critical(nullptr, "Critical error", "Path cannot be accessed.");
			return 1;
		}
	
	}
	else if (stat(argv[1], &file_stat) != 0) // else read cmd line args and stat the first one
	{
		QMessageBox::critical(nullptr, "Critical error", "Path cannot be accessed.");
		return 1;
	}
    
	MainWindow window(app.arguments(), file_stat, stdin_buffer);
	window.setWindowIcon(QIcon::fromTheme("view-process-users"));
	window.exec();
    return 0;
}
