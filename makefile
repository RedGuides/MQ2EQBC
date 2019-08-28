!include "../global.mak"

ALL : "$(OUTDIR)\MQ2EQBC.dll"

CLEAN :
	-@erase "$(INTDIR)\MQ2EQBC.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MQ2EQBC.dll"
	-@erase "$(OUTDIR)\MQ2EQBC.exp"
	-@erase "$(OUTDIR)\MQ2EQBC.lib"
	-@erase "$(OUTDIR)\MQ2EQBC.pdb"


LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(DETLIB) ..\Release\MQ2Main.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\MQ2EQBC.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MQ2EQBC.dll" /implib:"$(OUTDIR)\MQ2EQBC.lib" /OPT:NOICF /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\MQ2EQBC.obj" \
	"$(OUTDIR)\MQ2Main.lib"

"$(OUTDIR)\MQ2EQBC.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MQ2EQBC.dep")
!INCLUDE "MQ2EQBC.dep"
!ELSE 
!MESSAGE Warning: cannot find "MQ2EQBC.dep"
!ENDIF 
!ENDIF 


SOURCE=.\MQ2EQBC.cpp

"$(INTDIR)\MQ2EQBC.obj" : $(SOURCE) "$(INTDIR)"

