#ifndef JOINROOM_HPP
#define JOINROOM_HPP


using namespace DuiLib;

class JoinRoom : public WindowImplBase
{
public:
	JoinRoom();

	AString getServer();
	AString getRoomName();

protected:

	LPCTSTR GetWindowClassName() const;

	virtual void OnFinalMessage(HWND hWnd);

	void Notify(TNotifyUI& msg);

	virtual void InitWindow();

	virtual CDuiString GetSkinFile();

	virtual CDuiString GetSkinFolder();

private:
	CEditUI *edit_server;
	CEditUI *edit_room;
	AString m_server;
	AString m_room;
};

#endif // JOINROOM_HPP
