// Generic EPOC32 launcher: Foo.app starts Foo.exe from the same folder.
#include <eikapp.h>
#include <eikdoc.h>
#include <eikappui.h>
#include <eikenv.h>
#include <eikcmds.hrh>

class CLaunchAppUi : public CEikAppUi
{
public:
	void ConstructL()
	{
		BaseConstructL();
		TFileName exe = Application()->AppFullName();
		TInt dot = exe.LocateReverse('.');
		if (dot >= 0) exe.SetLength(dot);
		exe.Append(_L(".exe"));
		RProcess p;
		TInt err = p.Create(exe, KNullDesC);
		if (err == KErrNone) {
			p.Resume();
			p.Close();
		} else {
			TBuf<64> msg;
			msg.Format(_L("Launch failed: %d"), err);
			iEikonEnv->InfoMsg(msg);
			User::After(3000000);
		}
		Exit();
	}
	void HandleCommandL(TInt aCommand) { if (aCommand == EEikCmdExit) Exit(); }
};

class CLaunchDocument : public CEikDocument
{
public:
	CLaunchDocument(CEikApplication& aApp) : CEikDocument(aApp) {}
	CEikAppUi* CreateAppUiL() { return new(ELeave) CLaunchAppUi; }
};

class CLaunchApplication : public CEikApplication
{
	CApaDocument* CreateDocumentL() { return new(ELeave) CLaunchDocument(*this); }
	TUid AppDllUid() const { return TUid::Uid(UID3); }
};

EXPORT_C CApaApplication* NewApplication() { return new CLaunchApplication; }
GLDEF_C TInt E32Dll(TDllReason) { return KErrNone; }
