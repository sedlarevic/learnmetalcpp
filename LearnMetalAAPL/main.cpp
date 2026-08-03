//
//  main.cpp
//  LearnMetalAAPL
//
//  Created by Stefan Sedlarevic on 2. 8. 2026..
//

#include <iostream>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#pragma region Declarations {

class Renderer{
public:
    Renderer( MTL::Device* pDevice );
    ~Renderer();
    void draw( MTK::View* pView );
    
private:
    MTL::Device* _pDevice;
    MTL::CommandQueue* _pCommandQueue;
};

class MTKViewDelegate : public MTK::ViewDelegate{
public:
    MTKViewDelegate( MTL::Device* pDevice );
    virtual ~MTKViewDelegate() override;
    virtual void drawInMTKView( MTK::View* pView ) override;
    
private:
    Renderer* _pRenderer;
};

class AppDelegate : public NS::ApplicationDelegate
{
public:
    ~AppDelegate();
    NS::Menu* createMenuBar();
    virtual void applicationWillFinishLaunching(NS::Notification* pNotification) override;
    virtual void applicationDidFinishLaunching(NS::Notification* pNotification) override;
    virtual bool applicationShouldTerminateAfterLastWindowClosed(NS::Application* pSender) override;
    
private:
    NS::Window* _pWindow;
    MTK::View* _pMtkView;
    MTL::Device* _pDevice;
    MTKViewDelegate* _pViewDelegate = nullptr;
};

#pragma endregion Declarations }

int main(int argc, const char * argv[]) {
    
    NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();
    
    AppDelegate delegate;
    
    NS::Application* pSharedApplication = NS::Application::sharedApplication();
    pSharedApplication->setDelegate(&delegate);
    pSharedApplication->run();
    
    pAutoreleasePool->release();
    
    return 0;
}

#pragma mark - AppDelegate
#pragma region AppDelegate {

AppDelegate::~AppDelegate()
{
    _pMtkView->release();
    _pWindow->release();
    _pDevice->release();
    
    delete _pViewDelegate;
}

NS::Menu* AppDelegate::createMenuBar()
{
    using NS::StringEncoding::UTF8StringEncoding;
    
    NS::Menu* pMainMenu = NS::Menu::alloc()->init();
    NS::MenuItem* pAppMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* pAppMenu = NS::Menu::alloc()->init( NS::String::string( "Appname", UTF8StringEncoding ));
    
    NS::String* appName = NS::RunningApplication::currentApplication()->localizedName();
    NS::String* quitItemName = NS::String::string("Quit ", UTF8StringEncoding)->stringByAppendingString(appName);
    
    SEL quitCb = NS::MenuItem::registerActionCallback("appQuit", [](void*,SEL,const NS::Object* pSender){
        auto pApp = NS::Application::sharedApplication();
        pApp->terminate( pSender );
    });
    
    NS::MenuItem* pAppQuitItem = pAppMenu->addItem( quitItemName, quitCb, NS::String::string("q", UTF8StringEncoding));
    pAppQuitItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );
    pAppMenuItem->setSubmenu( pAppMenu );
    
    NS::MenuItem* pWindowMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* pWindowMenu = NS::Menu::alloc()->init( NS::String::string("Window", UTF8StringEncoding));
    
    SEL closeWindowCb = NS::MenuItem::registerActionCallback("WindowClose", [](void*, SEL, const NS::Object*){
        
        auto pApp = NS::Application::sharedApplication();
        pApp->windows()->object< NS::Window >(0)->close();
    });
    
    NS::MenuItem* pCloseWindowItem = pWindowMenu->addItem( NS::String::string("Close Window", UTF8StringEncoding), closeWindowCb, NS::String::string("w",UTF8StringEncoding));
    pCloseWindowItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);
    
    pWindowMenuItem->setSubmenu(pWindowMenu);
    
    pMainMenu->addItem( pAppMenuItem );
    pMainMenu->addItem( pWindowMenuItem );
    
    pAppMenuItem->release();
    pWindowMenuItem->release();
    pAppMenu->release();
    pWindowMenu->release();
    
    
    return pMainMenu->autorelease();
}

void AppDelegate::applicationWillFinishLaunching(NS::Notification *pNotification){
    
    NS::Menu* pMenu = createMenuBar();
    NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
    pApp->setMainMenu( pMenu );
    pApp->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);
}

void AppDelegate::applicationDidFinishLaunching(NS::Notification *pNotification){
    CGRect frame = (CGRect){{100.0, 100.0},{512.0, 512.0}};
    
    _pWindow = NS::Window::alloc()->init(
                                         frame,
                                         NS::WindowStyleMaskClosable|NS::WindowStyleMaskTitled,
                                         NS::BackingStoreBuffered,
                                         false
                                         );
    _pDevice = MTL::CreateSystemDefaultDevice();
    
    _pMtkView = MTK::View::alloc()->init(frame, _pDevice);
    _pMtkView->setColorPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    _pMtkView->setClearColor( MTL::ClearColor::Make(1.0,0.0,0.0,1.0));
    
    _pViewDelegate = new MTKViewDelegate( _pDevice );
    _pMtkView->setDelegate( _pViewDelegate );
    
    _pWindow->setContentView( _pMtkView );
    _pWindow->setTitle(NS::String::string("00 - Window", NS::StringEncoding::UTF8StringEncoding));
    
    _pWindow->makeKeyAndOrderFront(nullptr);
    
    NS::Application* pApp = reinterpret_cast<NS::Application* >(pNotification->object());
    pApp->activateIgnoringOtherApps(true);
}

bool AppDelegate::applicationShouldTerminateAfterLastWindowClosed(NS::Application *pSender){
    return true;
}
#pragma endregion AppDelegate }

#pragma mark - ViewDelegate
#pragma region ViewDelegate {

MTKViewDelegate::MTKViewDelegate(MTL::Device* pDevice) : MTK::ViewDelegate(), _pRenderer(new Renderer(pDevice)){}

MTKViewDelegate::~MTKViewDelegate(){
    delete _pRenderer;
}
void MTKViewDelegate::drawInMTKView(MTK::View* pView){
    _pRenderer->draw( pView );
}

#pragma endregion ViewDelegate }

#pragma mark - Renderer
#pragma region Renderer {

Renderer::Renderer( MTL::Device* pDevice ) : _pDevice( pDevice->retain() ){
    _pCommandQueue = _pDevice->newCommandQueue();
}

Renderer::~Renderer(){
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::draw( MTK::View* pView ){
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
    
    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );
    pEnc->endEncoding();
    pCmd->presentDrawable( pView->currentDrawable());
    pCmd->commit();
    
    pPool->release();
}
#pragma endregion Renderer }
