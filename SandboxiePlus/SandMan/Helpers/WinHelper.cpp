#include "stdafx.h"
#include "WinHelper.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtWin>
#else
#include <windows.h>
#endif

#include <Shlwapi.h>
#include <Shlobj.h>

QString ResolveShortcutUrl(const QString& LinkPath)
{
    QUrl Url;

    IShellLink* pShellLink = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<void**>(&pShellLink));

    if (SUCCEEDED(hr)) 
    {
        IPersistFile* pPersistFile = nullptr;
        hr = pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pPersistFile));

        if (SUCCEEDED(hr)) 
        {
            hr = pPersistFile->Load(LinkPath.toStdWString().c_str(), STGM_READ);

            if (SUCCEEDED(hr)) 
            {
                PIDLIST_ABSOLUTE pidl;
                hr = pShellLink->GetIDList(&pidl);

                if (SUCCEEDED(hr)) 
                {
                    LPWSTR url = nullptr;
                    SHGetNameFromIDList(pidl, SIGDN_URL, &url);
                    
                    if (url) {
                        Url = QString::fromWCharArray(url);
                        
                        CoTaskMemFree(url);
                    }

                    CoTaskMemFree(pidl);
                }
            }

            pPersistFile->Release();
        }

        pShellLink->Release();
    }

    if (Url.isLocalFile())
        return Url.path().mid(1).replace("/", "\\");
    return Url.toString();;
}

QVariantMap ResolveShortcut(const QString& LinkPath)
{
    QVariantMap Link;

    HRESULT hRes = E_FAIL;
    IShellLink* psl = NULL;

    // buffer that receives the null-terminated string
    // for the drive and path
    TCHAR szPath[0x1000];
    // structure that receives the information about the shortcut
    WIN32_FIND_DATA wfd;

    // Get a pointer to the IShellLink interface
    hRes = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl);

    if (SUCCEEDED(hRes))
    {
        // Get a pointer to the IPersistFile interface
        IPersistFile*  ppf     = NULL;
        psl->QueryInterface(IID_IPersistFile, (void **) &ppf);

        // Open the shortcut file and initialize it from its contents
        hRes = ppf->Load(LinkPath.toStdWString().c_str(), STGM_READ);
        if (SUCCEEDED(hRes))
        {
            hRes = psl->Resolve(NULL, SLR_NO_UI | SLR_NOSEARCH | SLR_NOUPDATE);
            if (SUCCEEDED(hRes))
            {
                // Get the path to the shortcut target
                hRes = psl->GetPath(szPath, ARRAYSIZE(szPath), &wfd, SLGP_RAWPATH);
                if (hRes == S_OK)
                    Link["Path"] = QString::fromWCharArray(szPath);
                else
                    Link["Path"] = ResolveShortcutUrl(LinkPath);

                hRes = psl->GetArguments(szPath, ARRAYSIZE(szPath));
                if (!FAILED(hRes))
                    Link["Arguments"] = QString::fromWCharArray(szPath);

                hRes = psl->GetWorkingDirectory(szPath, ARRAYSIZE(szPath));
                if (!FAILED(hRes))
				    Link["WorkingDir"] = QString::fromWCharArray(szPath);

				int IconIndex;
                hRes = psl->GetIconLocation(szPath, ARRAYSIZE(szPath), &IconIndex);
                if (FAILED(hRes))
                    return Link;
				Link["IconPath"] = QString::fromWCharArray(szPath);
				Link["IconIndex"] = IconIndex;

                // Get the description of the target
                hRes = psl->GetDescription(szPath, ARRAYSIZE(szPath));
                if (FAILED(hRes))
                    return Link;
                Link["Info"] = QString::fromWCharArray(szPath);
            }
        }
    }

    return Link;
}

QPixmap LoadWindowsIcon(const QString& Path, quint32 Index)
{
	std::wstring path = QString(Path).replace("/", "\\").toStdWString();
	HICON icon = ExtractIconW(NULL, path.c_str(), Index);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QPixmap Icon = QtWin::fromHICON(icon);
#else
	QPixmap Icon = QPixmap::fromImage(QImage::fromHICON(icon));
#endif
	DestroyIcon(icon);
	return Icon;
}

bool PickWindowsIcon(QWidget* pParent, QString& Path, quint32& Index)
{
	wchar_t iconPath[MAX_PATH] = { 0 };
	Path.toWCharArray(iconPath);
	BOOL Ret = PickIconDlg((HWND)pParent->window()->winId(), iconPath, MAX_PATH, (int*)&Index);
	Path = QString::fromWCharArray(iconPath);
	return !!Ret;
}