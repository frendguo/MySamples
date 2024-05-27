#include <sensorsapi.h>
#include <sensors.h>
#include <atlbase.h> // 用于CComObject和COM相关功能
#include <iostream>

// 定义事件监听器类
class CSensorEventListener : public ISensorEvents
{
public:
    ULONG _cRef;

    CSensorEventListener() : _cRef(1) {}

    // IUnknown方法
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (ppv == nullptr) {
            return E_POINTER;
        }
        *ppv = nullptr;
        if (riid == __uuidof(ISensorEvents)) {
            *ppv = static_cast<ISensorEvents*>(this);
        }
        else if (riid == IID_IUnknown) {
            *ppv = static_cast<IUnknown*>(this);
        }
        if (*ppv != nullptr) {
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }
    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = InterlockedDecrement(&_cRef);
        if (0 == cRef) {
            delete this;
        }
        return cRef;
    }

    // ISensorEvents方法
    IFACEMETHODIMP OnEvent(ISensor* pSensor, REFGUID eventID, IPortableDeviceValues* pEventData)
    {
        std::cout << "Event received" << std::endl;
        // 事件处理代码
        return S_OK;
    }
    IFACEMETHODIMP OnDataUpdated(ISensor* pSensor, ISensorDataReport* pNewData)
    {
        PROPVARIANT varValue;
        PropVariantInit(&varValue);
        pNewData->GetSensorValue(SENSOR_DATA_TYPE_LIGHT_LEVEL_LUX, &varValue);
        std::cout << "Light level: " << varValue.fltVal << " lux" << std::endl;
        // 数据更新处理代码，例如读取亮度值
        return S_OK;
    }
    IFACEMETHODIMP OnLeave(REFSENSOR_ID sensorID) { return S_OK; }
    IFACEMETHODIMP OnStateChanged(ISensor* pSensor, SensorState state) { return S_OK; }
};

void RegisterSensorEvent()
{
    // 初始化COM
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ISensorManager* pSensorManager = NULL;

    // 创建传感器管理器实例
    HRESULT hr = CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pSensorManager));
    if (SUCCEEDED(hr))
    {
        ISensorCollection* pSensorCollection = NULL;
        hr = pSensorManager->GetSensorsByType(SENSOR_TYPE_AMBIENT_LIGHT, &pSensorCollection);
        if (SUCCEEDED(hr))
        {
            ULONG count = 0;
            pSensorCollection->GetCount(&count);
            if (count > 0)
            {
                ISensor* pLightSensor = NULL;
                hr = pSensorCollection->GetAt(0, &pLightSensor);
                if (SUCCEEDED(hr))
                {
                    // 创建事件监听器实例并注册
                    CSensorEventListener* pEventListener = new CSensorEventListener();
                    hr = pLightSensor->SetEventSink(pEventListener);
                    if (SUCCEEDED(hr))
                    {
                        std::cout << "Event sink registered" << std::endl;
                    }
                    pLightSensor->Release();
                }
            }
            pSensorCollection->Release();
        }
        pSensorManager->Release();
    }
    CoUninitialize();
}

int main()
{
    RegisterSensorEvent();
    
    while (true)
    {
        Sleep(1000);
    }
    return 0;
}
