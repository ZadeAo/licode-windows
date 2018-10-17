#ifndef ABOUTME_HPP
#define ABOUTME_HPP


using namespace DuiLib;

class AboutMe : public WindowImplBase
{
public:
	AboutMe();

protected:

	LPCTSTR GetWindowClassName() const;

	virtual void OnFinalMessage(HWND hWnd);

	void Notify(TNotifyUI& msg);

	virtual void InitWindow();

	virtual CDuiString GetSkinFile();

	virtual CDuiString GetSkinFolder();

private:
};

#endif // ABOUTME_HPP
