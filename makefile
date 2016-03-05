out\EasyCKL.dll : default out\simple_app.obj out\simple_handler.obj out\simple_handler_win.obj out\CKLMain.obj cefsimple.res
	link /nologo /DLL /DEF:"Export.def" out\*.obj cefsimple.res /out:"out\EasyCKL.dll" /LIBPATH:".\lib"

out\simple_app.obj : simple_app.cpp
	cl /nologo /EHsc /MT /c simple_app.cpp /Fo:out\simple_app.obj

out\simple_handler.obj : simple_handler.cpp
	cl /nologo /EHsc /MT /c simple_handler.cpp /Fo:out\simple_handler.obj

out\simple_handler_win.obj : simple_handler_win.cpp
	cl /nologo /EHsc /MT /c simple_handler_win.cpp /Fo:out\simple_handler_win.obj

out\CKLMain.obj : CKLMain.cpp
	cl /nologo /EHsc /MT /c CKLMain.cpp /Fo:out\CKLMain.obj

cefsimple.res : cefsimple.rc
	rc cefsimple.rc

clean:
	del /f /q out\*.*
	del /f cefsimple.res

default:
	if not exist "out" mkdir out