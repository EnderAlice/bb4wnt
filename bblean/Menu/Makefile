NAME = menu
OUTPUT = $(NAME).lib

OBJS = MenuMaker.obj MenuItem.obj Menu.obj TitleItem.obj FolderItem.obj CommandItem.obj SpecialFolder.obj Contextmenu.obj Dragsource.obj Droptarget.obj RecentItem.obj
LIBS = 

DEFINES = /DOEMRESOURCE /DNTDDI_VERSION=NTDDI_WIN7 /D_WIN32_WINNT=0x0601 /D_UNICODE /DUNICODE

AFLAGS = $(DEFINES)
CFLAGS = $(DEFINES) /nologo /MD /GF /W3 /Og /Ot /GL /Gy /Gs /Z7 /Yd /hotpatch
#CFLAGS = $(DEFINES) /nologo /MDd /GF /W3 /GL /Gy /Gs /Z7 /Yd /hotpatch /RTCs
CPPFLAGS = $(CFLAGS)
LIBFLAGS = /NOLOGO /LTCG /OUT:..\$(OUTPUT)

!IF "$(AS)" == "ml"
CFLAGS = $(CFLAGS) /arch:SSE2
CPPFLAGS = $(CPPFLAGS) /arch:SSE2
!ENDIF

all: $(OBJS)
	lib $(LIBFLAGS) $(OBJS)

clean:
	cmd /c del $(OBJS) $(NAME).lib $(NAME).exp $(NAME).pdb vc*.pdb $(OUTPUT) $(OUTPUT).manifest
