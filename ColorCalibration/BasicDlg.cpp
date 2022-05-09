// BasicDlg.cpp : implementation file
//
//BIG5 TRANS ALLOWED

#include "stdafx.h"
#include "Basic.h"
#include "windows.h"
#include "afxwin.h"
#include "afxdialogex.h"
#include <atlimage.h>
#include "BasicDlg.h"
#include "malloc.h"
#include <opencv.hpp>
#include <opencv2/mcc.hpp>
#include "color_calibration.h"
#include "color_checker.h"
#include "color_correction.h"
#include "color_science.h"
#include <functional>
#include <numeric> 
#include "serial.h"
#include "json.hpp"
#include <time.h>
using Json = nlohmann::json;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern int gLanguage;

//using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

/*相机配置窗口的消息回调函数
hCamera:当前相机的句柄
MSG:消息类型，
	SHEET_MSG_LOAD_PARAM_DEFAULT	= 0,//加载默认参数的按钮被点击，加载默认参数完成后触发该消息,
	SHEET_MSG_LOAD_PARAM_GROUP		= 1,//切换参数组完成后触发该消息,
	SHEET_MSG_LOAD_PARAM_FROMFILE	= 2,//加载参数按钮被点击，已从文件中加载相机参数后触发该消息
	SHEET_MSG_SAVE_PARAM_GROUP		= 3//保存参数按钮被点击，参数保存后触发该消息
	具体参见CameraDefine.h中emSdkPropSheetMsg类型

uParam:消息附带的参数，不同的消息，参数意义不同。
	当 MSG 为 SHEET_MSG_LOAD_PARAM_DEFAULT时，uParam表示被加载成默认参数组的索引号，从0开始，分别对应A,B,C,D四组
	当 MSG 为 SHEET_MSG_LOAD_PARAM_GROUP时，uParam表示切换后的参数组的索引号，从0开始，分别对应A,B,C,D四组
	当 MSG 为 SHEET_MSG_LOAD_PARAM_FROMFILE时，uParam表示被文件中参数覆盖的参数组的索引号，从0开始，分别对应A,B,C,D四组
	当 MSG 为 SHEET_MSG_SAVE_PARAM_GROUP时，uParam表示当前保存的参数组的索引号，从0开始，分别对应A,B,C,D四组
*/


//****************************************************************************************

void MatToCImage(cv::Mat& mat, CImage& cImage)
{

	int width = mat.cols;
	int height = mat.rows;
	int channels = mat.channels();

	cImage.Destroy();//这一步是防止重复利用造成内存问题
	cImage.Create(width, height, 8 * channels);

	uchar* ps;
	uchar* pimg = (uchar*)cImage.GetBits(); //获取CImage的像素存贮区的指针
	int step = cImage.GetPitch();//每行的字节数,注意这个返回值有正有负

	// 如果是1个通道的图像(灰度图像) DIB格式才需要对调色板设置  
	// CImage中内置了调色板，我们要对他进行赋值：
	if (1 == channels)
	{
		RGBQUAD* ColorTable;
		int MaxColors = 256;
		//这里可以通过CI.GetMaxColorTableEntries()得到大小(如果你是CI.Load读入图像的话)  
		ColorTable = new RGBQUAD[MaxColors];
		cImage.GetColorTable(0, MaxColors, ColorTable);//这里是取得指针  
		for (int i = 0; i < MaxColors; i++)
		{
			ColorTable[i].rgbBlue = (BYTE)i;
			//BYTE和uchar一回事，但MFC中都用它  
			ColorTable[i].rgbGreen = (BYTE)i;
			ColorTable[i].rgbRed = (BYTE)i;
		}
		cImage.SetColorTable(0, MaxColors, ColorTable);
		delete[]ColorTable;
	}


	for (int i = 0; i < height; i++)
	{
		ps = mat.ptr<uchar>(i);
		for (int j = 0; j < width; j++)
		{
			if (1 == channels)
			{
				*(pimg + i * step + j) = ps[j];
				//*(pimg + i*step + j) = 105;
			}
			else if (3 == channels)
			{
				*(pimg + i * step + j * 3) = ps[j * 3];
				*(pimg + i * step + j * 3 + 1) = ps[j * 3 + 1];
				*(pimg + i * step + j * 3 + 2) = ps[j * 3 + 2];
			}
		}
	}
}

void MatToByte(cv::Mat srcImg, BYTE*& pImg)
{
	int nFlag = srcImg.channels() * 8;//一个像素的bits
	int nHeight = srcImg.rows;
	int nWidth = srcImg.cols;

	int nBytes = nHeight * nWidth * nFlag / 8;//图像总的字节
	if (pImg)
		delete[] pImg;
	pImg = new BYTE[nBytes];//new的单位为字节
	memcpy(pImg, srcImg.data, nBytes);//转化函数,注意Mat的data成员	
}

bool CBasicDlg::calibration(cv::Mat& image, cv::Mat& detected_image, cv::Mat& calibrated_image)
{
	
	cv::Mat image_normalization = image.clone();
	auto color_checker_func = ColorCheckerFunc();
	auto color_calibration = ColorCalibration();
	auto color_correction = ColorCorrection();
	auto t1 = clock();
	color_checker_func.DetectChecker(image_normalization, m_config.input_image_format, m_config.color_checker_resize_ratio);
	auto t2 = clock();
	auto centroid = color_checker_func.centroid;
	if (color_checker_func.is_colorchecker)
	{
		ShowImageInImageControl(color_checker_func.detected_image, IDC_STATIC);
		auto charts_linearrgb = color_checker_func.original_charts_linearrgb;
		auto charts_srgb = color_checker_func.original_charts_srgb;
		detected_image = color_checker_func.detected_image;
		float wb_gain[3];
		color_calibration.CalculateWBGain(charts_linearrgb, wb_gain);
		auto wb_charts_linearrgb = charts_linearrgb.clone();
		for (auto i = 0; i < 24; i++) 
		{
			wb_charts_linearrgb.at<cv::Vec3d>(0, i)[0] *= wb_gain[0];
			wb_charts_linearrgb.at<cv::Vec3d>(0, i)[1] *= wb_gain[1];
			wb_charts_linearrgb.at<cv::Vec3d>(0, i)[2] *= wb_gain[2];
		}
		std::cout << wb_gain[0] << std::endl;
		std::cout << wb_gain[1] << std::endl;
		std::cout << wb_gain[2] << std::endl;



		float Y_gain = color_calibration.CalculateYGain(wb_charts_linearrgb);
		auto y_wb_charts_linearrgb = wb_charts_linearrgb.clone();
		y_wb_charts_linearrgb = y_wb_charts_linearrgb * Y_gain;
		double cct = color_calibration.CalculateCCT(wb_gain);


		cv::Mat ccm;
		if (m_config.ccm_position == 1)
		{
			cv::Mat y_wb_charts_srgb;
			cv::pow(y_wb_charts_linearrgb, (1 / 2.2), y_wb_charts_srgb);
			ccm = color_calibration.CalculateCCM(y_wb_charts_srgb, m_config.ccm_format, m_config.ColorChecker2005_3nh);
		}
		else
		{
			ccm = color_calibration.CalculateCCM(y_wb_charts_linearrgb, m_config.ccm_format, m_config.ColorChecker2005_imatest);
		}
		auto t3 = clock();
		std::cout << t3 - t2 << std::endl;
		std::cout << t2 - t1 << std::endl;


		std::cout << "cct:" << cct << std::endl;
		//std::cout << "ccm:" << ccm << std::endl;

		color_calibration.WriteConfig(m_config.ccm_result_directory, ccm, cct, m_config.ccm_format);

		//std::cout << "R GAIN:" << wb_gain[0] << std::endl;
		//std::cout << "G GAIN:" << wb_gain[1] << std::endl;
		//std::cout << "B GAIN:" << wb_gain[2] << std::endl;
		color_correction.CorrectImage(image_normalization, 0, calibrated_image, 1, 1, 1, m_config.ccm_position, wb_gain, Y_gain, ccm, 0);
		//auto color_science = ColorScience();
		std::vector<double> delta_E;
		std::vector<double> delta_C;
		EvaluateImage(calibrated_image, 0, delta_E, delta_C);
		double sum = std::accumulate(std::begin(delta_E), std::end(delta_E), 0.0);
		double mean = sum / delta_E.size();
		std::cout << "mean delta_E:" << mean << std::endl;
		sum = std::accumulate(std::begin(delta_C), std::end(delta_C), 0.0);
		mean = sum / delta_C.size();
		mean = sum / delta_C.size();
		std::cout << "mean delta_C:" << mean << std::endl;

		cv::pow(calibrated_image, (1 / 2.2), calibrated_image);
		calibrated_image.convertTo(calibrated_image, CV_8UC3, 255);
		//for (auto i = 0; i < 24; i++)
		//{
		//	std::cout << centroid[i] << std::endl;
		//	cv::putText(calibrated_image, std::to_string(int(10*color_science.delta_E[i])), centroid[i], cv::FONT_HERSHEY_COMPLEX, 2, cv::Scalar(0, 255, 255));
		//}
		return TRUE;
	}
	else
	{
		GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("未检测到色卡"));
		UpdateData(FALSE);
		MessageBox("未检测到色卡");
		return FALSE;
	}
}

//****************************************************************************************


void _stdcall CameraSettingPageCallback(CameraHandle hCamera,UINT MSG,UINT uParam,PVOID pContext)
{
	CBasicDlg *pDialog = (CBasicDlg*)pContext;
	CString sMsg;

	if (MSG == SHEET_MSG_LOAD_PARAM_DEFAULT)
	{
		sMsg.Format("Parameter group[%d] was loaded to default!",uParam);
	}
	else if (MSG == SHEET_MSG_LOAD_PARAM_GROUP)
	{
		sMsg.Format("Parameter group[%d] was loaded!",uParam);
	} 
	else if (MSG == SHEET_MSG_LOAD_PARAM_FROMFILE)
	{
		sMsg.Format("Parameter group[%d] was loaded from file!",uParam);
	}
	else if (MSG == SHEET_MSG_SAVE_PARAM_GROUP)
	{
		sMsg.Format("Parameter group[%d] was saved!",uParam);
	}
	else
	{
		return;//unknown message type
	}
	pDialog->m_DlgLog.AppendLog("CallBack: CameraSettingPageCallback");
	pDialog->m_DlgLog.AppendLog(sMsg);
	pDialog->m_DlgLog.AppendLog(" ");

}

/*
USE_CALLBACK_GRAB_IMAGE
如果需要使用回调函数的方式获得图像数据，则反注释宏定义USE_CALLBACK_GRAB_IMAGE.
我们的SDK同时支持回调函数和主动调用接口抓取图像的方式。两种方式都采用了"零拷贝"机制，以最大的程度的降低系统负荷，提高程序执行效率。
但是主动抓取方式比回调函数的方式更加灵活，可以设置超时等待时间等，我们建议您使用 uiDisplayThread 中的方式
*/
//#define USE_CALLBACK_GRAB_IMAGE 

#ifdef USE_CALLBACK_GRAB_IMAGE
/*图像抓取回调函数*/
void _stdcall GrabImageCallback(CameraHandle hCamera, BYTE *pFrameBuffer, tSdkFrameHead* pFrameHead,PVOID pContext)
{
	
	CameraSdkStatus status;
	CBasicDlg *pThis = (CBasicDlg*)pContext;
	
	//将获得的原始数据转换成RGB格式的数据，同时经过ISP模块，对图像进行降噪，边沿提升，颜色校正等处理。
	//我公司大部分型号的相机，原始数据都是Bayer格式的
	status = CameraImageProcess(pThis->m_hCamera, pFrameBuffer, pThis->m_pFrameBuffer,pFrameHead);

	//分辨率改变了，则刷新背景
	if (pThis->m_sFrInfo.iWidth != pFrameHead->iWidth || pThis->m_sFrInfo.iHeight != pFrameHead->iHeight)
	{
		pThis->m_sFrInfo.iWidth = pFrameHead->iWidth;
		pThis->m_sFrInfo.iHeight = pFrameHead->iHeight;
		pThis->InvalidateRect(NULL);//切换分辨率大小时，擦除背景。
	}
	
	if(status == CAMERA_STATUS_SUCCESS && !pThis->m_bPause)
    {
    	//调用SDK封装好的显示接口来显示图像,您也可以将m_pFrameBuffer中的RGB数据通过其他方式显示，比如directX,OpengGL,等方式。
		CameraImageOverlay(pThis->m_hCamera, pThis->m_pFrameBuffer,pFrameHead);
        CameraDisplayRGB24(pThis->m_hCamera, pThis->m_pFrameBuffer, pFrameHead);//这里可以替换成用户自己的显示函数
        pThis->m_iDispFrameNum++;
		pThis->ProcessSnapRequest(pThis->m_pFrameBuffer, pFrameHead);
    }    
    
	memcpy(&pThis->m_sFrInfo,pFrameHead,sizeof(tSdkFrameHead));
	
}

#else 
/*图像抓取线程，主动调用SDK接口函数获取图像*/
UINT WINAPI uiDisplayThread(LPVOID lpParam)
{
	tSdkFrameHead 	sFrameInfo;
	CBasicDlg* 		pThis = (CBasicDlg*)lpParam;
	BYTE*			pbyBuffer;
	CameraSdkStatus status;
	
	while (!pThis->m_bExit)
    {   

		if(CameraGetImageBufferPriority(pThis->m_hCamera,&sFrameInfo,&pbyBuffer,1000,
			CAMERA_GET_IMAGE_PRIORITY_NEWEST) == CAMERA_STATUS_SUCCESS)
		{	
			//将获得的原始数据转换成RGB格式的数据，同时经过ISP模块，对图像进行降噪，边沿提升，颜色校正等处理。
			//我公司大部分型号的相机，原始数据都是Bayer格式的
            //std::cout << "success" << std::endl;
			// 
            //float ccm[9] = {2, 0, 0, 0, 0.1, 0, 0, 0, 1};
            //CameraSetUserClrTempMatrix(pThis->m_hCamera, ccm);
			//CameraSetAeState(pThis->m_hCamera, TRUE);
			//CameraSetAeTarget(pThis->m_hCamera, 50);
			status = CameraImageProcess(pThis->m_hCamera, pbyBuffer, pThis->m_pFrameBuffer,&sFrameInfo);//连续模式
			//分辨率改变了，则刷新背景


			if (pThis->m_sFrInfo.iWidth != sFrameInfo.iWidth || pThis->m_sFrInfo.iHeight != sFrameInfo.iHeight)
			{
				pThis->m_sFrInfo.iWidth = sFrameInfo.iWidth;
				pThis->m_sFrInfo.iHeight = sFrameInfo.iHeight;
				pThis->InvalidateRect(NULL);
			}
			
			if(status == CAMERA_STATUS_SUCCESS)
            {
            	//调用SDK封装好的显示接口来显示图像,您也可以将m_pFrameBuffer中的RGB数据通过其他方式显示，比如directX,OpengGL,等方式。
				CameraImageOverlay(pThis->m_hCamera, pThis->m_pFrameBuffer, &sFrameInfo);
                CameraDisplayRGB24(pThis->m_hCamera, pThis->m_pFrameBuffer, &sFrameInfo);
                pThis->m_iDispFrameNum++;
				pThis->ProcessSnapRequest(pThis->m_pFrameBuffer, &sFrameInfo);
            }    
            
			//在成功调用CameraGetImageBuffer后，必须调用CameraReleaseImageBuffer来释放获得的buffer。
			//否则再次调用CameraGetImageBuffer时，程序将被挂起，知道其他线程中调用CameraReleaseImageBuffer来释放了buffer
            CameraReleaseImageBuffer(pThis->m_hCamera,pbyBuffer);
            
			memcpy(&pThis->m_sFrInfo,&sFrameInfo,sizeof(tSdkFrameHead));
		}
		
    }
	
	_endthreadex(0);
    return 0;
}
#endif

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CString	m_sAboutInfo;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	DECLARE_MESSAGE_MAP()
};


CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	m_sAboutInfo = _T("This example shows how to integrate the camera into your system in a very fast and easy way!");
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_EDIT_ABOUT, m_sAboutInfo);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBasicDlg dialog

CBasicDlg::CBasicDlg(CWnd* pParent /*=NULL*/)
: CDialog(IDD_BASIC_DIALOG_CN, pParent)
{
	//{{AFX_DATA_INIT(CBasicDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
	m_bPause = TRUE;
	m_hCamera = -1;
	m_iDispFrameNum = 0;
	m_bExit = FALSE;
	m_hDispThread = NULL;
	m_pFrameBuffer = NULL;
	m_SnapRequest = 0;
}

void CBasicDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBasicDlg)
	DDX_Control(pDX, IDC_STATIC_PREVIEW, m_cPreview);
	//DDX_Control(pDX, IDC_STATIC, m_cPreview);

	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CBasicDlg, CDialog)
	//{{AFX_MSG_MAP(CBasicDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_PREVIEW, OnButtonPreview)

	ON_BN_CLICKED(IDC_BUTTON_AE, &CBasicDlg::OnBnClickedButtonAe)
	ON_BN_CLICKED(IDC_BUTTON_CALIBRATION, &CBasicDlg::OnBnClickedButtonCalibration)
	ON_BN_CLICKED(IDC_BUTTON_CC_DETECT, &CBasicDlg::OnBnClickedButtonCcDetect)
	ON_BN_CLICKED(IDC_BUTTON_ONCE_CALIBRATION, &CBasicDlg::OnBnClickedButtonOnceCalibration)


	//ON_BN_CLICKED(IDC_BUTTON_CAMERA_SETTINGS, OnButtonCameraSettings)
	//ON_BN_CLICKED(IDC_BUTTON_SNAPSHOT, OnButtonSnapshot)
	//ON_BN_CLICKED(IDC_BUTTON_SNAPSHOT_FAST, OnButtonSnapshotFast)
	//ON_BN_CLICKED(IDC_BUTTON_ABOUT, OnButtonAbout)
	//ON_WM_CLOSE()
	//ON_WM_TIMER()
	//}}AFX_MSG_MAP
	//ON_STN_CLICKED(IDC_STATIC, &CBasicDlg::OnStnClickedStaticShow)
	//ON_STN_CLICKED(IDC_STATIC2, &CBasicDlg::OnStnClickedStatic2)
	//ON_WM_LBUTTONDOWN()
	//ON_BN_CLICKED(IDC_BUTTON1, &CBasicDlg::OnBnClickedButton1)
	//ON_BN_CLICKED(IDC_BUTTON2, &CBasicDlg::OnBnClickedButton2)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBasicDlg message handlers

BOOL CBasicDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	if (!m_config._debug)
	{
		GetDlgItem(IDC_BUTTON_AE)->ShowWindow(SW_HIDE);
		//GetDlgItem(IDC_BUTTON_CALIBRATION)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUTTON_CC_DETECT)->ShowWindow(SW_HIDE);
	}
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//m_DlgLog.Create(IDD_DIALOG_LOG,this);//创建一个信息窗口来显示日志
	do 
	{
		m_DlgLog.ShowWindow(SW_SHOW);
		
// 		m_DlgLog.AppendLog("Basic Demo start");
// 		m_DlgLog.AppendLog("LoadSdkApi was called to load SDK api from MVCAMSDK.dll");
// 		m_DlgLog.AppendLog("LoadSdkApi is open source in CameraApiLoad.h ");
// 		m_DlgLog.AppendLog("It shows how to load the api from MVCAMSDK.dll,");
// 		m_DlgLog.AppendLog("you can also use your own way to load MVCAMSDK.dll");


		//Init the SDK，0:英文版 1:中文版 ，作用于相机的描述信息和SDK生成的设备配置界面上
		SDK_TRACE(CameraSdkInit(gLanguage),"初始化SDK");

		if (!InitCamera())
		{
			break;
		}
		
		SetTimer(0,1000,NULL);//使用一个定时器来计算帧率

		return TRUE;

	} while(0);
	
	//没有找到相机或者初始化失败，退出程序
	EndDialog(0);
	return FALSE;  // return TRUE  unless you set the focus to a control
}

void CBasicDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CBasicDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);

		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}


// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CBasicDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


void CBasicDlg::OnButtonPreview() 
{
	m_bPause = !m_bPause;
	GetDlgItem(IDC_BUTTON_PREVIEW)->SetWindowText(m_bPause?"开始预览":"停止预览");
	if (m_bPause)
	{
		//Set the camera in pause mode
		SDK_TRACE(CameraPause(m_hCamera),"暂停相机工作");
	}
	else
	{
		//Set the camera in play mode
		SDK_TRACE(CameraPlay(m_hCamera),"开始预览");
	}
}


void CBasicDlg::ShowImageInImageControl(cv::Mat& image, int place)
{
	m_DlgLog.AppendLog("**************im here******************");
	CWnd* pWnd = GetDlgItem(place);
	CDC* pDc = GetDlgItem(place)->GetDC();
	CImage image2;
	MatToCImage(image, image2);
	CRect rectControl;
	pWnd->GetClientRect(rectControl);
	auto height = rectControl.Height();
	int width = int(float(height) / float(image.rows) * float(image.cols));
	auto rect1 = CRect(rectControl.TopLeft(), CSize(width, height));
	pDc->SetStretchBltMode(STRETCH_HALFTONE);
	image2.Draw(pDc->m_hDC, rect1);
	image2.Destroy();
	pWnd->ReleaseDC(pDc);
}


cv::Mat CBasicDlg::GetImageFromCamera()
{
	//auto image = cv::imread("C:/Users/30880/Desktop/mindvision/mindvision/SM-SUA501GC-T-Snapshot-20220407-155513-744-6178296820.PNG");
	//return image;
	tSdkFrameHead FrameInfo;
	BYTE* pRawBuffer;
	BYTE* pRgbBuffer;
	CString sFileName;
	tSdkImageResolution sImageSize;
	CameraSdkStatus status;
	CString msg;
	memset(&sImageSize, 0, sizeof(tSdkImageResolution));
	sImageSize.iIndex = 0xff;
	//CameraSetResolutionForSnap设置抓拍时的分辨率，可以和预览时相同，也可以不同。
	//sImageSize.iIndex = 0xff; sImageSize.iWidth 和 sImageSize.iHeight 置0，表示抓拍时分辨率和预览时相同。
	//如果您希望抓拍时为单独的分辨率，请参考我们的Snapshot例程。或者参阅SDK手册中有关CameraSetResolutionForSnap接口的详细说明
	SDK_TRACE(CameraSetResolutionForSnap(m_hCamera, &sImageSize), "设置抓拍模式下的分辨率");

	//	CameraSnapToBuffer抓拍一帧图像数据到缓冲区中，该缓冲区由SDK内部申请,成功调用后，需要
	// !!!!!!注意：CameraSnapToBuffer 会切换分辨率拍照，速度较慢。做实时处理，请用CameraGetImageBuffer函数取图或者回调函数。
	if ((status = CameraSnapToBuffer(m_hCamera, &FrameInfo, &pRawBuffer, 1000)) != CAMERA_STATUS_SUCCESS)
	{
		m_DlgLog.AppendLog("Function:[CameraSnapToBuffer] failed!");
		msg.Format("Error Code:%d. Get more information about the error in CameraDefine.h", status);
		m_DlgLog.AppendLog(msg);
		m_DlgLog.AppendLog(" ");
		MessageBox("获取图像失败， 请开启预览模式！");
		GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("获取图像失败,请开启预览模式!!!"));
		UpdateData(FALSE);
		CStatic* pStatic1 = (CStatic*)GetDlgItem(IDC_STATIC);
		pStatic1->SetBitmap(NULL);
		RedrawWindow();
		CStatic* pStatic2 = (CStatic*)GetDlgItem(IDC_STATIC2);
		pStatic2->SetBitmap(NULL);
		RedrawWindow();
		return cv::Mat();


	}
	else
	{
		msg.Format("Function:[%s] SUCCESS!", "CameraSnapToBuffer");
		m_DlgLog.AppendLog(msg);
		msg.Format("Description:%s.", "Capture a image to the buffer in snapshot mode");
		m_DlgLog.AppendLog(msg);
		m_DlgLog.AppendLog(" ");

		//成功抓拍后，保存到文件
		CString msg;
		char sCurpath[MAX_PATH];
		CString strTime = CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S"));
		GetCurrentDirectory(MAX_PATH, sCurpath);
		sFileName.Format("%s\\Snapshot%s", sCurpath, strTime);//初始化保存文件的文件名

		//申请一个buffer，用来将获得的原始数据转换为RGB数据，并同时获得图像处理效果
		pRgbBuffer = (BYTE*)CameraAlignMalloc(FrameInfo.iWidth * FrameInfo.iHeight * 4, 16);
		//Process the raw data,and get the return image in RGB format
		SDK_TRACE(CameraImageProcess(m_hCamera, pRawBuffer, pRgbBuffer, &FrameInfo),"处理图像，并得到RGB格式的数据");

		CameraFlipFrameBuffer(pRgbBuffer, &FrameInfo, 1);

		cv::Mat matImage(
			cv::Size(FrameInfo.iWidth, FrameInfo.iHeight),
			FrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? CV_8UC1 : CV_8UC3,
			pRgbBuffer
		);
		SDK_TRACE(CameraReleaseImageBuffer(m_hCamera, pRawBuffer), "释放由CameraSnapToBuffer、CameraGetImageBuffer获得的图像缓冲区");
		//CameraAlignFree(pRgbBuffer);
		matImage -= m_config.blc;
		return matImage;
	}
}


void CBasicDlg::ProcessSnapRequest(BYTE* pImageData, tSdkFrameHead *pImageHead)
{
	if (m_SnapRequest == 0)
		return;
	InterlockedDecrement(&m_SnapRequest);

	static ULONG s_Index = 0;
	CString strFileName;
	GetCurrentDirectory(MAX_PATH, strFileName.GetBufferSetLength(MAX_PATH));
	strFileName.ReleaseBuffer();
	strFileName.AppendFormat("\\Snapshot%u", s_Index++);

	CameraSaveImage(m_hCamera, strFileName.GetBuffer(), pImageData, pImageHead, FILE_BMP, 100);
}

void CBasicDlg::OnButtonAbout() 
{
	CAboutDlg dlg;
	dlg.DoModal();
}


BOOL CBasicDlg::InitCamera()
{
	tSdkCameraDevInfo sCameraList[10];
	INT iCameraNums;
	CameraSdkStatus status;
	CRect rect;
	tSdkCameraCapbility sCameraInfo;
	
	//枚举设备，获得设备列表
	//Enumerate camera
	iCameraNums = 10;//调用CameraEnumerateDevice前，先设置iCameraNums = 10，表示最多只读取10个设备，如果需要枚举更多的设备，请更改sCameraList数组的大小和iCameraNums的值
	

	//std::ofstream fout("C:/Users/30880/Desktop/mindvision/1.bin", std::ios::binary);
	//unsigned short nNum = 1;
	//fout.write((char*)&nNum, sizeof(unsigned short));
	//fout.close();

	if (CameraEnumerateDevice(sCameraList,&iCameraNums) != CAMERA_STATUS_SUCCESS || iCameraNums == 0)
	{
		MessageBox("请连接相机!!!");
		return TRUE;
	}

	//该示例中，我们只假设连接了一个相机。因此，只初始化第一个相机。(-1,-1)表示加载上次退出前保存的参数，如果是第一次使用该相机，则加载默认参数.
	//In this demo ,we just init the first camera.
	if ((status = CameraInit(&sCameraList[0],-1,-1,&m_hCamera)) != CAMERA_STATUS_SUCCESS)
	{
		CString msg;
		msg.Format("无法连接相机! 错误码: %d",status);
		MessageBox(msg + "，原因是" + CameraGetErrorString(status));
		return FALSE;
	}
	
	CameraSetAeState(m_hCamera, TRUE);
	CameraSetAeTarget(m_hCamera, m_config.ae_time);
	int r=100, g=100, b=100;
	CameraSetGain(m_hCamera, r, g, b);
	CameraGetGain(m_hCamera, &r, &g, &b);
	std::cout << "r gain:" << r << std::endl;
	std::cout << "g gain:" << g << std::endl;
	std::cout << "b gain:" << b << std::endl;


	//Get properties description for this camera.
	SDK_TRACE(CameraGetCapability(m_hCamera,&sCameraInfo),"获得该相机的特性描述");

	m_pFrameBuffer = (BYTE *)CameraAlignMalloc(sCameraInfo.sResolutionRange.iWidthMax*sCameraInfo.sResolutionRange.iHeightMax*4,16);	

	ASSERT(m_pFrameBuffer);

	if (sCameraInfo.sIspCapacity.bMonoSensor)
	{
		// 让黑白相机最终输出MONO8，如果不设置，则CameraImageProcess会默认输出24位的灰度图
		SDK_TRACE(CameraSetIspOutFormat(m_hCamera, CAMERA_MEDIA_TYPE_MONO8),"黑白相机设置为输出灰度数据");
	}
	
	//使用SDK封装好的显示接口
	//Use Mindvision SDK to display camera images.
	SDK_TRACE(CameraDisplayInit(m_hCamera,m_cPreview.GetSafeHwnd()),"初始化显示接口");
	m_cPreview.GetClientRect(&rect);
	//Set display window size
	SDK_TRACE(CameraSetDisplaySize(m_hCamera,rect.right - rect.left,rect.bottom - rect.top),"设置显示窗口大小");
	
	//通知SDK内部建该相机的属性页面。窗口名称为该相机的名称。您也可以根据SDK的接口来自己实现相机的属性配置页面，
	//但是我们建议您使用SDK内部自动创建的方式，来省去您在界面开发上的大量时间。
	//Create the settings window for the camera
	SDK_TRACE(CameraCreateSettingPage(m_hCamera,GetSafeHwnd(),
								sCameraList[0].acFriendlyName,CameraSettingPageCallback,(PVOID)this,0)
								,"通知SDK内部建该相机的属性页面");
 
	#ifdef USE_CALLBACK_GRAB_IMAGE //如果要使用回调函数方式，定义USE_CALLBACK_GRAB_IMAGE这个宏
	//Set the callback for image capture



	SDK_TRACE(CameraSetCallbackFunction(m_hCamera,GrabImageCallback,(PVOID)this,NULL),"设置图像抓取的回调函数");
	#else
	m_hDispThread = (HANDLE)_beginthreadex(NULL, 0, &uiDisplayThread, this, 0,  &m_threadID);
	ASSERT (m_hDispThread); 
	SetThreadPriority(m_hDispThread,THREAD_PRIORITY_HIGHEST);
	#endif
	//Tell the camera begin to sendding image
	SDK_TRACE(CameraPlay(m_hCamera),"开始预览");
	m_bPause = FALSE;
	GetDlgItem(IDC_BUTTON_PREVIEW)->SetWindowText("停止预览");
	return TRUE;
}


void CBasicDlg::OnBnClickedButtonAe()
{
	// TODO: 在此添加控件通知处理程序代码
	// TODO: do calibration AE
	CameraSetAeState(m_hCamera, TRUE);
	CameraSetAeTarget(m_hCamera, m_config.ae_time);
	double ex_time;
	CameraGetExposureTime(m_hCamera, &ex_time);
	cv::Mat image = GetImageFromCamera();

	if (image.data) {
		if (m_config.color_checker_resize_ratio != 1)
		{
			cv::resize(image, image, cv::Size(), m_config.color_checker_resize_ratio, 0.25);
		}
		image.convertTo(image, CV_32FC3, (1 / 255.0));
		auto color_checker_func = ColorCheckerFunc();
		color_checker_func.DetectChecker(image, 0, m_config.color_checker_resize_ratio);
		if (color_checker_func.is_colorchecker)
		{

			auto white_color = color_checker_func.original_charts_linearrgb.at<cv::Vec3d>(0, 18);
			auto max_value = max(white_color[0], max(white_color[1], white_color[2]));
			double new_exposure_time = (m_config.ae_time * 0.8) / max_value;
			CameraSetAeState(m_hCamera, TRUE);
			CameraSetExposureTime(m_hCamera, new_exposure_time);
			std::cout << "max color:" << max_value << std::endl;
			std::cout << "calib ae time:" << m_config.ae_time << std::endl;

		}
		else
		{
			MessageBox("检测不到色卡!!!无法完成AE");
		}
	}
}


//void CBasicDlg::OnBnClickedButton2()
//{
//	// TODO: 在此添加控件通知处理程序代码
//	cv::Mat image = GetImageFromCamera();
//	if (image.data) {
//		if (m_config.color_checker_resize_ratio != 1)
//		{
//			cv::resize(image, image, cv::Size(), m_config.color_checker_resize_ratio, m_config.color_checker_resize_ratio);
//		}
//		image.convertTo(image, CV_32FC3, (1 / 255.0));
//		auto color_checker_func = ColorCheckerFunc();
//		color_checker_func.DetectChecker(image, 0, m_config.color_checker_resize_ratio);
//		if (color_checker_func.is_colorchecker)
//		{
//			cv::Mat detected_image = color_checker_func.detected_image;
//			detected_image.convertTo(detected_image, CV_8UC3);
//			ShowImageInImageControl(detected_image, IDC_STATIC);
//		}
//		else
//		{
//			MessageBox("检测不到色卡!!!");
//		}
//	}
//}


void CBasicDlg::OnBnClickedButtonCalibration()
{

	Json j;			// 创建 json 对象
	ifstream jfile("C:/Users/30880/Desktop/demo/calibration_config.json");
	jfile >> j;		// 以文件流形式读取 json 文件
	string str = j.at("result_directory");
	m_config.ccm_result_directory = str;
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("获取图像..."));
	UpdateData(FALSE);
	cv::Mat matImage = GetImageFromCamera();
	//matImage = cv::imread("C:/Users/30880/Desktop/mindvision/SM-SUA501GC-T-Snapshot-20220407-155513-744-6178296820.PNG");

	auto time_start = GetTickCount();
	std::string result_path = "C:/Users/30880/Desktop/calibration_result/raw_" + std::to_string(time_start) + ".png";
	cv::imwrite(result_path, matImage);

	if (matImage.data) {
		GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("标定中..."));
		UpdateData(FALSE);
		matImage.convertTo(matImage, CV_32FC3, (1 / 255.0 / m_config.calib_image_num));
		for (auto i = 0; i < m_config.calib_image_num - 1; i++)
		{
			cv::Mat mat = GetImageFromCamera();
			mat.convertTo(mat, CV_32FC3, (1 / 255.0 / m_config.calib_image_num));
			matImage += mat;
		}
		cv::Mat detected_image;
		cv::Mat calibrated_image;
		bool is_calib = this->calibration(matImage, detected_image, calibrated_image);
		ShowImageInImageControl(calibrated_image, IDC_STATIC2);
		std::string result_path2 = "C:/Users/30880/Desktop/calibration_result/calib_" + std::to_string(time_start) + ".png";
		cv::imwrite(result_path2, calibrated_image);

		if (is_calib)
		{
			GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("标定成功!!!"));
			UpdateData(FALSE);
			m_DlgLog.AppendLog("calibration success!!!");
		}
		else
		{
			CStatic* pStatic = (CStatic*)GetDlgItem(IDC_STATIC);
			pStatic->SetBitmap(NULL);
			RedrawWindow();
			GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("未检测到色卡, 标定失败!!!"));
			UpdateData(FALSE);
			//CStatic* pStatic1 = (CStatic*)GetDlgItem(IDC_STATIC);
			//pStatic1->SetBitmap(NULL);
			//CStatic* pStatic2 = (CStatic*)GetDlgItem(IDC_STATIC2);
			//pStatic2->SetBitmap(NULL);
			MessageBox("标定失败!!!");
			m_DlgLog.AppendLog("calibration fail!!!  can not find colorchecker");
		}
	}
}





void CBasicDlg::OnBnClickedButtonCcDetect()
{
	// TODO: 在此添加控件通知处理程序代码
	cv::Mat image = GetImageFromCamera();
	if (image.data) {
		if (m_config.color_checker_resize_ratio != 1)
		{
			cv::resize(image, image, cv::Size(), m_config.color_checker_resize_ratio, m_config.color_checker_resize_ratio);
		}
		image.convertTo(image, CV_32FC3, (1 / 255.0));
		auto color_checker_func = ColorCheckerFunc();
		color_checker_func.DetectChecker(image, 0, m_config.color_checker_resize_ratio);
		if (color_checker_func.is_colorchecker)
		{
			cv::Mat detected_image = color_checker_func.detected_image;
			detected_image.convertTo(detected_image, CV_8UC3);
			ShowImageInImageControl(detected_image, IDC_STATIC);
		}
		else
		{
			MessageBox("检测不到色卡!!!");
		}
	}
}


	void CBasicDlg::OnBnClickedButtonOnceCalibration()
	{
		// TODO: 在此添加控件通知处理程序代码CSerialPort mySerialPort;//首先将之前定义的类实例化
		CSerialPort mySerialPort;//首先将之前定义的类实例化

		if (!mySerialPort.InitPort(m_config.com_num, CBR_9600, 'N', 8, 1, EV_RXCHAR))//是否打开串口，3就是你外设连接电脑的com口，可以在设备管理器查看，然后更改这个参数
		{
			std::cout << "initPort fail !" << std::endl;
			MessageBox("未连接灯箱或串口不正确！！！");
			return;
		}
		else
		{
			std::cout << "initPort success !" << std::endl;
		}
		if (!mySerialPort.OpenListenThread())//是否打开监听线程，开启线程用来传输返回值
		{
			std::cout << "OpenListenThread fail !" << std::endl;
		}
		else
		{
			std::cout << "OpenListenThread success !" << std::endl;
		}
		std::string str = "<1,S=ON>\n";
		const char* p = str.data();
		std::cout << mySerialPort.WriteData(p, str.size() - 1) << std::endl;//这个函数就是给串口发送数据的函数，temp就是要发送的数组。
		std::cout << mySerialPort.GetBytesInCOM() << std::endl;//这个函数就是显示返回值函数
		std::cout << "******************************" << std::endl;
		Sleep(5000);

		for (auto i = 0; i <= 100; i += 10)
		{
			std::cout << "******************************" << std::endl;
			str = "<1,C=" + to_string(i) + ">\n";
			std::cout << str << std::endl;
			const char* p2 = str.data();
			mySerialPort.WriteData(p2, str.size() - 1);
			std::cout << mySerialPort.GetBytesInCOM() << std::endl;//这个函数就是显示返回值函数
			//Sleep(2000);
			//OnBnClickedButtonAe();
			Sleep(3000);
			OnBnClickedButtonCalibration();
			Sleep(1000);
		}

		std::string str2 = "<1,S=OFF>\n";
		const char* p2 = str2.data();
		std::cout << mySerialPort.WriteData(p2, str2.size() - 1) << std::endl;//这个函数就是给串口发送数据的函数，temp就是要发送的数组。
		std::cout << mySerialPort.GetBytesInCOM() << std::endl;//这个函数就是显示返回值函数
	}
