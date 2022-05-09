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

/*������ô��ڵ���Ϣ�ص�����
hCamera:��ǰ����ľ��
MSG:��Ϣ���ͣ�
	SHEET_MSG_LOAD_PARAM_DEFAULT	= 0,//����Ĭ�ϲ����İ�ť�����������Ĭ�ϲ�����ɺ󴥷�����Ϣ,
	SHEET_MSG_LOAD_PARAM_GROUP		= 1,//�л���������ɺ󴥷�����Ϣ,
	SHEET_MSG_LOAD_PARAM_FROMFILE	= 2,//���ز�����ť��������Ѵ��ļ��м�����������󴥷�����Ϣ
	SHEET_MSG_SAVE_PARAM_GROUP		= 3//���������ť���������������󴥷�����Ϣ
	����μ�CameraDefine.h��emSdkPropSheetMsg����

uParam:��Ϣ�����Ĳ�������ͬ����Ϣ���������岻ͬ��
	�� MSG Ϊ SHEET_MSG_LOAD_PARAM_DEFAULTʱ��uParam��ʾ�����س�Ĭ�ϲ�����������ţ���0��ʼ���ֱ��ӦA,B,C,D����
	�� MSG Ϊ SHEET_MSG_LOAD_PARAM_GROUPʱ��uParam��ʾ�л���Ĳ�����������ţ���0��ʼ���ֱ��ӦA,B,C,D����
	�� MSG Ϊ SHEET_MSG_LOAD_PARAM_FROMFILEʱ��uParam��ʾ���ļ��в������ǵĲ�����������ţ���0��ʼ���ֱ��ӦA,B,C,D����
	�� MSG Ϊ SHEET_MSG_SAVE_PARAM_GROUPʱ��uParam��ʾ��ǰ����Ĳ�����������ţ���0��ʼ���ֱ��ӦA,B,C,D����
*/


//****************************************************************************************

void MatToCImage(cv::Mat& mat, CImage& cImage)
{

	int width = mat.cols;
	int height = mat.rows;
	int channels = mat.channels();

	cImage.Destroy();//��һ���Ƿ�ֹ�ظ���������ڴ�����
	cImage.Create(width, height, 8 * channels);

	uchar* ps;
	uchar* pimg = (uchar*)cImage.GetBits(); //��ȡCImage�����ش�������ָ��
	int step = cImage.GetPitch();//ÿ�е��ֽ���,ע���������ֵ�����и�

	// �����1��ͨ����ͼ��(�Ҷ�ͼ��) DIB��ʽ����Ҫ�Ե�ɫ������  
	// CImage�������˵�ɫ�壬����Ҫ�������и�ֵ��
	if (1 == channels)
	{
		RGBQUAD* ColorTable;
		int MaxColors = 256;
		//�������ͨ��CI.GetMaxColorTableEntries()�õ���С(�������CI.Load����ͼ��Ļ�)  
		ColorTable = new RGBQUAD[MaxColors];
		cImage.GetColorTable(0, MaxColors, ColorTable);//������ȡ��ָ��  
		for (int i = 0; i < MaxColors; i++)
		{
			ColorTable[i].rgbBlue = (BYTE)i;
			//BYTE��ucharһ���£���MFC�ж�����  
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
	int nFlag = srcImg.channels() * 8;//һ�����ص�bits
	int nHeight = srcImg.rows;
	int nWidth = srcImg.cols;

	int nBytes = nHeight * nWidth * nFlag / 8;//ͼ���ܵ��ֽ�
	if (pImg)
		delete[] pImg;
	pImg = new BYTE[nBytes];//new�ĵ�λΪ�ֽ�
	memcpy(pImg, srcImg.data, nBytes);//ת������,ע��Mat��data��Ա	
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
		GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("δ��⵽ɫ��"));
		UpdateData(FALSE);
		MessageBox("δ��⵽ɫ��");
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
�����Ҫʹ�ûص������ķ�ʽ���ͼ�����ݣ���ע�ͺ궨��USE_CALLBACK_GRAB_IMAGE.
���ǵ�SDKͬʱ֧�ֻص��������������ýӿ�ץȡͼ��ķ�ʽ�����ַ�ʽ��������"�㿽��"���ƣ������ĳ̶ȵĽ���ϵͳ���ɣ���߳���ִ��Ч�ʡ�
��������ץȡ��ʽ�Ȼص������ķ�ʽ�������������ó�ʱ�ȴ�ʱ��ȣ����ǽ�����ʹ�� uiDisplayThread �еķ�ʽ
*/
//#define USE_CALLBACK_GRAB_IMAGE 

#ifdef USE_CALLBACK_GRAB_IMAGE
/*ͼ��ץȡ�ص�����*/
void _stdcall GrabImageCallback(CameraHandle hCamera, BYTE *pFrameBuffer, tSdkFrameHead* pFrameHead,PVOID pContext)
{
	
	CameraSdkStatus status;
	CBasicDlg *pThis = (CBasicDlg*)pContext;
	
	//����õ�ԭʼ����ת����RGB��ʽ�����ݣ�ͬʱ����ISPģ�飬��ͼ����н��룬������������ɫУ���ȴ���
	//�ҹ�˾�󲿷��ͺŵ������ԭʼ���ݶ���Bayer��ʽ��
	status = CameraImageProcess(pThis->m_hCamera, pFrameBuffer, pThis->m_pFrameBuffer,pFrameHead);

	//�ֱ��ʸı��ˣ���ˢ�±���
	if (pThis->m_sFrInfo.iWidth != pFrameHead->iWidth || pThis->m_sFrInfo.iHeight != pFrameHead->iHeight)
	{
		pThis->m_sFrInfo.iWidth = pFrameHead->iWidth;
		pThis->m_sFrInfo.iHeight = pFrameHead->iHeight;
		pThis->InvalidateRect(NULL);//�л��ֱ��ʴ�Сʱ������������
	}
	
	if(status == CAMERA_STATUS_SUCCESS && !pThis->m_bPause)
    {
    	//����SDK��װ�õ���ʾ�ӿ�����ʾͼ��,��Ҳ���Խ�m_pFrameBuffer�е�RGB����ͨ��������ʽ��ʾ������directX,OpengGL,�ȷ�ʽ��
		CameraImageOverlay(pThis->m_hCamera, pThis->m_pFrameBuffer,pFrameHead);
        CameraDisplayRGB24(pThis->m_hCamera, pThis->m_pFrameBuffer, pFrameHead);//��������滻���û��Լ�����ʾ����
        pThis->m_iDispFrameNum++;
		pThis->ProcessSnapRequest(pThis->m_pFrameBuffer, pFrameHead);
    }    
    
	memcpy(&pThis->m_sFrInfo,pFrameHead,sizeof(tSdkFrameHead));
	
}

#else 
/*ͼ��ץȡ�̣߳���������SDK�ӿں�����ȡͼ��*/
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
			//����õ�ԭʼ����ת����RGB��ʽ�����ݣ�ͬʱ����ISPģ�飬��ͼ����н��룬������������ɫУ���ȴ���
			//�ҹ�˾�󲿷��ͺŵ������ԭʼ���ݶ���Bayer��ʽ��
            //std::cout << "success" << std::endl;
			// 
            //float ccm[9] = {2, 0, 0, 0, 0.1, 0, 0, 0, 1};
            //CameraSetUserClrTempMatrix(pThis->m_hCamera, ccm);
			//CameraSetAeState(pThis->m_hCamera, TRUE);
			//CameraSetAeTarget(pThis->m_hCamera, 50);
			status = CameraImageProcess(pThis->m_hCamera, pbyBuffer, pThis->m_pFrameBuffer,&sFrameInfo);//����ģʽ
			//�ֱ��ʸı��ˣ���ˢ�±���


			if (pThis->m_sFrInfo.iWidth != sFrameInfo.iWidth || pThis->m_sFrInfo.iHeight != sFrameInfo.iHeight)
			{
				pThis->m_sFrInfo.iWidth = sFrameInfo.iWidth;
				pThis->m_sFrInfo.iHeight = sFrameInfo.iHeight;
				pThis->InvalidateRect(NULL);
			}
			
			if(status == CAMERA_STATUS_SUCCESS)
            {
            	//����SDK��װ�õ���ʾ�ӿ�����ʾͼ��,��Ҳ���Խ�m_pFrameBuffer�е�RGB����ͨ��������ʽ��ʾ������directX,OpengGL,�ȷ�ʽ��
				CameraImageOverlay(pThis->m_hCamera, pThis->m_pFrameBuffer, &sFrameInfo);
                CameraDisplayRGB24(pThis->m_hCamera, pThis->m_pFrameBuffer, &sFrameInfo);
                pThis->m_iDispFrameNum++;
				pThis->ProcessSnapRequest(pThis->m_pFrameBuffer, &sFrameInfo);
            }    
            
			//�ڳɹ�����CameraGetImageBuffer�󣬱������CameraReleaseImageBuffer���ͷŻ�õ�buffer��
			//�����ٴε���CameraGetImageBufferʱ�����򽫱�����֪�������߳��е���CameraReleaseImageBuffer���ͷ���buffer
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

	//m_DlgLog.Create(IDD_DIALOG_LOG,this);//����һ����Ϣ��������ʾ��־
	do 
	{
		m_DlgLog.ShowWindow(SW_SHOW);
		
// 		m_DlgLog.AppendLog("Basic Demo start");
// 		m_DlgLog.AppendLog("LoadSdkApi was called to load SDK api from MVCAMSDK.dll");
// 		m_DlgLog.AppendLog("LoadSdkApi is open source in CameraApiLoad.h ");
// 		m_DlgLog.AppendLog("It shows how to load the api from MVCAMSDK.dll,");
// 		m_DlgLog.AppendLog("you can also use your own way to load MVCAMSDK.dll");


		//Init the SDK��0:Ӣ�İ� 1:���İ� �������������������Ϣ��SDK���ɵ��豸���ý�����
		SDK_TRACE(CameraSdkInit(gLanguage),"��ʼ��SDK");

		if (!InitCamera())
		{
			break;
		}
		
		SetTimer(0,1000,NULL);//ʹ��һ����ʱ��������֡��

		return TRUE;

	} while(0);
	
	//û���ҵ�������߳�ʼ��ʧ�ܣ��˳�����
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
	GetDlgItem(IDC_BUTTON_PREVIEW)->SetWindowText(m_bPause?"��ʼԤ��":"ֹͣԤ��");
	if (m_bPause)
	{
		//Set the camera in pause mode
		SDK_TRACE(CameraPause(m_hCamera),"��ͣ�������");
	}
	else
	{
		//Set the camera in play mode
		SDK_TRACE(CameraPlay(m_hCamera),"��ʼԤ��");
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
	//CameraSetResolutionForSnap����ץ��ʱ�ķֱ��ʣ����Ժ�Ԥ��ʱ��ͬ��Ҳ���Բ�ͬ��
	//sImageSize.iIndex = 0xff; sImageSize.iWidth �� sImageSize.iHeight ��0����ʾץ��ʱ�ֱ��ʺ�Ԥ��ʱ��ͬ��
	//�����ϣ��ץ��ʱΪ�����ķֱ��ʣ���ο����ǵ�Snapshot���̡����߲���SDK�ֲ����й�CameraSetResolutionForSnap�ӿڵ���ϸ˵��
	SDK_TRACE(CameraSetResolutionForSnap(m_hCamera, &sImageSize), "����ץ��ģʽ�µķֱ���");

	//	CameraSnapToBufferץ��һ֡ͼ�����ݵ��������У��û�������SDK�ڲ�����,�ɹ����ú���Ҫ
	// !!!!!!ע�⣺CameraSnapToBuffer ���л��ֱ������գ��ٶȽ�������ʵʱ��������CameraGetImageBuffer����ȡͼ���߻ص�������
	if ((status = CameraSnapToBuffer(m_hCamera, &FrameInfo, &pRawBuffer, 1000)) != CAMERA_STATUS_SUCCESS)
	{
		m_DlgLog.AppendLog("Function:[CameraSnapToBuffer] failed!");
		msg.Format("Error Code:%d. Get more information about the error in CameraDefine.h", status);
		m_DlgLog.AppendLog(msg);
		m_DlgLog.AppendLog(" ");
		MessageBox("��ȡͼ��ʧ�ܣ� �뿪��Ԥ��ģʽ��");
		GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("��ȡͼ��ʧ��,�뿪��Ԥ��ģʽ!!!"));
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

		//�ɹ�ץ�ĺ󣬱��浽�ļ�
		CString msg;
		char sCurpath[MAX_PATH];
		CString strTime = CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S"));
		GetCurrentDirectory(MAX_PATH, sCurpath);
		sFileName.Format("%s\\Snapshot%s", sCurpath, strTime);//��ʼ�������ļ����ļ���

		//����һ��buffer����������õ�ԭʼ����ת��ΪRGB���ݣ���ͬʱ���ͼ����Ч��
		pRgbBuffer = (BYTE*)CameraAlignMalloc(FrameInfo.iWidth * FrameInfo.iHeight * 4, 16);
		//Process the raw data,and get the return image in RGB format
		SDK_TRACE(CameraImageProcess(m_hCamera, pRawBuffer, pRgbBuffer, &FrameInfo),"����ͼ�񣬲��õ�RGB��ʽ������");

		CameraFlipFrameBuffer(pRgbBuffer, &FrameInfo, 1);

		cv::Mat matImage(
			cv::Size(FrameInfo.iWidth, FrameInfo.iHeight),
			FrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? CV_8UC1 : CV_8UC3,
			pRgbBuffer
		);
		SDK_TRACE(CameraReleaseImageBuffer(m_hCamera, pRawBuffer), "�ͷ���CameraSnapToBuffer��CameraGetImageBuffer��õ�ͼ�񻺳���");
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
	
	//ö���豸������豸�б�
	//Enumerate camera
	iCameraNums = 10;//����CameraEnumerateDeviceǰ��������iCameraNums = 10����ʾ���ֻ��ȡ10���豸�������Ҫö�ٸ�����豸�������sCameraList����Ĵ�С��iCameraNums��ֵ
	

	//std::ofstream fout("C:/Users/30880/Desktop/mindvision/1.bin", std::ios::binary);
	//unsigned short nNum = 1;
	//fout.write((char*)&nNum, sizeof(unsigned short));
	//fout.close();

	if (CameraEnumerateDevice(sCameraList,&iCameraNums) != CAMERA_STATUS_SUCCESS || iCameraNums == 0)
	{
		MessageBox("���������!!!");
		return TRUE;
	}

	//��ʾ���У�����ֻ����������һ���������ˣ�ֻ��ʼ����һ�������(-1,-1)��ʾ�����ϴ��˳�ǰ����Ĳ���������ǵ�һ��ʹ�ø�����������Ĭ�ϲ���.
	//In this demo ,we just init the first camera.
	if ((status = CameraInit(&sCameraList[0],-1,-1,&m_hCamera)) != CAMERA_STATUS_SUCCESS)
	{
		CString msg;
		msg.Format("�޷��������! ������: %d",status);
		MessageBox(msg + "��ԭ����" + CameraGetErrorString(status));
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
	SDK_TRACE(CameraGetCapability(m_hCamera,&sCameraInfo),"��ø��������������");

	m_pFrameBuffer = (BYTE *)CameraAlignMalloc(sCameraInfo.sResolutionRange.iWidthMax*sCameraInfo.sResolutionRange.iHeightMax*4,16);	

	ASSERT(m_pFrameBuffer);

	if (sCameraInfo.sIspCapacity.bMonoSensor)
	{
		// �úڰ�����������MONO8����������ã���CameraImageProcess��Ĭ�����24λ�ĻҶ�ͼ
		SDK_TRACE(CameraSetIspOutFormat(m_hCamera, CAMERA_MEDIA_TYPE_MONO8),"�ڰ��������Ϊ����Ҷ�����");
	}
	
	//ʹ��SDK��װ�õ���ʾ�ӿ�
	//Use Mindvision SDK to display camera images.
	SDK_TRACE(CameraDisplayInit(m_hCamera,m_cPreview.GetSafeHwnd()),"��ʼ����ʾ�ӿ�");
	m_cPreview.GetClientRect(&rect);
	//Set display window size
	SDK_TRACE(CameraSetDisplaySize(m_hCamera,rect.right - rect.left,rect.bottom - rect.top),"������ʾ���ڴ�С");
	
	//֪ͨSDK�ڲ��������������ҳ�档��������Ϊ����������ơ���Ҳ���Ը���SDK�Ľӿ����Լ�ʵ���������������ҳ�棬
	//�������ǽ�����ʹ��SDK�ڲ��Զ������ķ�ʽ����ʡȥ���ڽ��濪���ϵĴ���ʱ�䡣
	//Create the settings window for the camera
	SDK_TRACE(CameraCreateSettingPage(m_hCamera,GetSafeHwnd(),
								sCameraList[0].acFriendlyName,CameraSettingPageCallback,(PVOID)this,0)
								,"֪ͨSDK�ڲ��������������ҳ��");
 
	#ifdef USE_CALLBACK_GRAB_IMAGE //���Ҫʹ�ûص�������ʽ������USE_CALLBACK_GRAB_IMAGE�����
	//Set the callback for image capture



	SDK_TRACE(CameraSetCallbackFunction(m_hCamera,GrabImageCallback,(PVOID)this,NULL),"����ͼ��ץȡ�Ļص�����");
	#else
	m_hDispThread = (HANDLE)_beginthreadex(NULL, 0, &uiDisplayThread, this, 0,  &m_threadID);
	ASSERT (m_hDispThread); 
	SetThreadPriority(m_hDispThread,THREAD_PRIORITY_HIGHEST);
	#endif
	//Tell the camera begin to sendding image
	SDK_TRACE(CameraPlay(m_hCamera),"��ʼԤ��");
	m_bPause = FALSE;
	GetDlgItem(IDC_BUTTON_PREVIEW)->SetWindowText("ֹͣԤ��");
	return TRUE;
}


void CBasicDlg::OnBnClickedButtonAe()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
			MessageBox("��ⲻ��ɫ��!!!�޷����AE");
		}
	}
}


//void CBasicDlg::OnBnClickedButton2()
//{
//	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
//			MessageBox("��ⲻ��ɫ��!!!");
//		}
//	}
//}


void CBasicDlg::OnBnClickedButtonCalibration()
{

	Json j;			// ���� json ����
	ifstream jfile("C:/Users/30880/Desktop/demo/calibration_config.json");
	jfile >> j;		// ���ļ�����ʽ��ȡ json �ļ�
	string str = j.at("result_directory");
	m_config.ccm_result_directory = str;
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("��ȡͼ��..."));
	UpdateData(FALSE);
	cv::Mat matImage = GetImageFromCamera();
	//matImage = cv::imread("C:/Users/30880/Desktop/mindvision/SM-SUA501GC-T-Snapshot-20220407-155513-744-6178296820.PNG");

	auto time_start = GetTickCount();
	std::string result_path = "C:/Users/30880/Desktop/calibration_result/raw_" + std::to_string(time_start) + ".png";
	cv::imwrite(result_path, matImage);

	if (matImage.data) {
		GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("�궨��..."));
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
			GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("�궨�ɹ�!!!"));
			UpdateData(FALSE);
			m_DlgLog.AppendLog("calibration success!!!");
		}
		else
		{
			CStatic* pStatic = (CStatic*)GetDlgItem(IDC_STATIC);
			pStatic->SetBitmap(NULL);
			RedrawWindow();
			GetDlgItem(IDC_STATIC_CALIB)->SetWindowText(_T("δ��⵽ɫ��, �궨ʧ��!!!"));
			UpdateData(FALSE);
			//CStatic* pStatic1 = (CStatic*)GetDlgItem(IDC_STATIC);
			//pStatic1->SetBitmap(NULL);
			//CStatic* pStatic2 = (CStatic*)GetDlgItem(IDC_STATIC2);
			//pStatic2->SetBitmap(NULL);
			MessageBox("�궨ʧ��!!!");
			m_DlgLog.AppendLog("calibration fail!!!  can not find colorchecker");
		}
	}
}





void CBasicDlg::OnBnClickedButtonCcDetect()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
			MessageBox("��ⲻ��ɫ��!!!");
		}
	}
}


	void CBasicDlg::OnBnClickedButtonOnceCalibration()
	{
		// TODO: �ڴ���ӿؼ�֪ͨ����������CSerialPort mySerialPort;//���Ƚ�֮ǰ�������ʵ����
		CSerialPort mySerialPort;//���Ƚ�֮ǰ�������ʵ����

		if (!mySerialPort.InitPort(m_config.com_num, CBR_9600, 'N', 8, 1, EV_RXCHAR))//�Ƿ�򿪴��ڣ�3�������������ӵ��Ե�com�ڣ��������豸�������鿴��Ȼ������������
		{
			std::cout << "initPort fail !" << std::endl;
			MessageBox("δ���ӵ���򴮿ڲ���ȷ������");
			return;
		}
		else
		{
			std::cout << "initPort success !" << std::endl;
		}
		if (!mySerialPort.OpenListenThread())//�Ƿ�򿪼����̣߳������߳��������䷵��ֵ
		{
			std::cout << "OpenListenThread fail !" << std::endl;
		}
		else
		{
			std::cout << "OpenListenThread success !" << std::endl;
		}
		std::string str = "<1,S=ON>\n";
		const char* p = str.data();
		std::cout << mySerialPort.WriteData(p, str.size() - 1) << std::endl;//����������Ǹ����ڷ������ݵĺ�����temp����Ҫ���͵����顣
		std::cout << mySerialPort.GetBytesInCOM() << std::endl;//�������������ʾ����ֵ����
		std::cout << "******************************" << std::endl;
		Sleep(5000);

		for (auto i = 0; i <= 100; i += 10)
		{
			std::cout << "******************************" << std::endl;
			str = "<1,C=" + to_string(i) + ">\n";
			std::cout << str << std::endl;
			const char* p2 = str.data();
			mySerialPort.WriteData(p2, str.size() - 1);
			std::cout << mySerialPort.GetBytesInCOM() << std::endl;//�������������ʾ����ֵ����
			//Sleep(2000);
			//OnBnClickedButtonAe();
			Sleep(3000);
			OnBnClickedButtonCalibration();
			Sleep(1000);
		}

		std::string str2 = "<1,S=OFF>\n";
		const char* p2 = str2.data();
		std::cout << mySerialPort.WriteData(p2, str2.size() - 1) << std::endl;//����������Ǹ����ڷ������ݵĺ�����temp����Ҫ���͵����顣
		std::cout << mySerialPort.GetBytesInCOM() << std::endl;//�������������ʾ����ֵ����
	}
