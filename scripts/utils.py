import tempfile, os, sys, subprocess

this_dir = os.path.dirname(os.path.abspath(__file__))
src_dir = os.path.dirname(this_dir) + "/src"

def red(text):
	return f"\033[31m{text}\033[0m"
	
def green(text):
	return f"\033[32m{text}\033[0m"
	
def blue(text):
	return f"\033[34m{text}\033[0m"
	
ok = green("[ OK ]")
fail = red("[ FAIL ]")
error = red("*** ERROR ***")

def is_accessible(directory):
	try:
		with tempfile.TemporaryFile(dir=directory):
			pass
	except:
		return False
		
	return True

def get_home_dir_from_argv():
	argv = sys.argv
	if len(argv) != 2:
		raise Exception("Invalid number of arguments. Expected 1 argument: home directory.")
	
	home_dir = argv[1]
	if not (is_accessible(home_dir)):
		raise Exception(f"{home_dir} is not writable or doesn't exist.")
		
	print(f"Home directory received: {home_dir} {ok}")
	return home_dir

def get_home_dir():
	return os.path.expanduser('~')

def get_system_plugin_dir():
	cmd = ["qtpaths6", "--plugin-dir"]
	result = subprocess.run(cmd, capture_output=True, text=True, check=True)
	stdout = result.stdout
	plugin_dir = stdout[:stdout.find("\n")]
	
	print(f"System plugin directory determined to be: {plugin_dir} {ok}")
	return plugin_dir
