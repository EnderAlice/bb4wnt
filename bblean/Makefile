NAME = blackbox
OUTPUT = $(NAME).exe

OBJS = Blackbox.obj BBApi.obj Utils.obj BImage.obj Settings.obj PluginManager.obj MessageManager.obj Workspaces.obj Tray.obj Desk.obj Toolbar.obj DesktopMenu.obj ConfigMenu.obj m_alloc.obj Tinylist.obj Install.obj BBRoot.obj BBVWM.obj Pidl.obj MinimizeToTray.obj VolumeControl.obj resource.res
LIBS = kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib uuid.lib comctl32.lib comdlg32.lib version.lib psapi.lib shlwapi.lib imm32.lib userenv.lib

DEFINES = /DOEMRESOURCE /DNTDDI_VERSION=NTDDI_WIN7 /D_WIN32_WINNT=0x0601 /DPSAPI_VERSION=1 /D_UNICODE /DUNICODE /D__BBCORE__

AFLAGS = $(DEFINES)
CFLAGS = $(DEFINES) /nologo /MD /GF /W3 /Og /Ot /GL /Gy /Gs /Z7 /Yd /hotpatch
#CFLAGS = $(DEFINES) /nologo /MDd /GF /W3 /GL /Gy /Gs /Z7 /Yd /hotpatch /RTCs
CPPFLAGS = $(CFLAGS)
RFLAGS = $(DEFINES) /nologo /n
LFLAGS = /NOLOGO /OUT:$(OUTPUT) /SUBSYSTEM:WINDOWS /LARGEADDRESSAWARE /LTCG /OPT:REF,ICF /FUNCTIONPADMIN /DEBUG

LFLAGS = $(LFLAGS)  /PDB:dust.pdb /PDBSTRIPPED:$(NAME).pdb /PDBALTPATH:$(NAME).pdb
#LFLAGS = $(LFLAGS)  /PDB:$(NAME).pdb /PDBALTPATH:$(NAME).pdb

!IF "$(AS)" == "ml"
CFLAGS = $(CFLAGS) /arch:SSE2
CPPFLAGS = $(CPPFLAGS) /arch:SSE2
!ENDIF

all: $(OBJS) menu.lib
	link $(LFLAGS) $(OBJS) menu.lib $(LIBS)
	<<tempfile.cmd
if not exist "$(OUTPUT).manifest" exit
echo LANGUAGE 0,0 >manifest.rc
echo 1 24 "$(OUTPUT).manifest">>manifest.rc
$(RC) /n /fo manifest.res manifest.rc
link /NOLOGO /OUT:$(OUTPUT) /MANIFEST:NO $(LFLAGS) $(OBJS) menu.lib $(LIBS) manifest.res
del manifest.rc manifest.res
<<
	-cmd /c del dust.pdb

menu.lib:
	cd Menu
	nmake
	cd ..

clean:
	cmd /c del $(OBJS) $(NAME).lib $(NAME).exp $(NAME).pdb vc*.pdb $(OUTPUT) $(OUTPUT).manifest
