#include <bdn/android/NavigationViewCore.h>
#include <bdn/android/wrapper/NativeNavigationView.h>
#include <bdn/ui/NavigationView.h>

namespace bdn::ui::detail
{
    CORE_REGISTER(NavigationView, bdn::ui::android::NavigationViewCore, NavigationView)
}

namespace bdn::ui::android
{
    NavigationViewCore::NavigationViewCore(const std::shared_ptr<ViewCoreFactory> &viewCoreFactory)
        : ViewCore(viewCoreFactory,
                   createAndroidViewClass<bdn::android::wrapper::NativeNavigationView>(viewCoreFactory))
    {
        geometry.onChange() += [=](auto &property) { this->reLayout(); };
    }

    NavigationViewCore::~NavigationViewCore() { getJViewAS<bdn::android::wrapper::NativeNavigationView>().close(); }

    void NavigationViewCore::pushView(std::shared_ptr<View> view, String title)
    {
        auto containerView = std::make_shared<ContainerView>(viewCoreFactory());
        containerView->isLayoutRoot = true;
        containerView->addChildView(view);
        containerView->offerLayout(layout());

        bool isFirst = _stack.empty();

        _stack.push_back({containerView, view, title});
        updateCurrentView(isFirst, true);
    }

    void NavigationViewCore::popView()
    {
        _stack.pop_back();
        updateCurrentView(false, false);
    }

    std::list<std::shared_ptr<View>> NavigationViewCore::childViews()
    {
        if (!_stack.empty()) {
            return {_stack.back().container};
        }
        return {};
    }

    void NavigationViewCore::visitInternalChildren(const std::function<void(std::shared_ptr<View::Core>)> &function)
    {
        for (const auto &entry : _stack) {
            function(entry.container->viewCore());
        }
    }

    void NavigationViewCore::updateCurrentView(bool first, bool enter)
    {
        if (!_stack.empty()) {
            auto fixedCore = _stack.back().container->core<android::ContainerViewCore>();

            auto NativeNavigationView = getJViewAS<bdn::android::wrapper::NativeNavigationView>();

            NativeNavigationView.setWindowTitle(_stack.back().title);
            NativeNavigationView.enableBackButton(_stack.size() > 1);
            NativeNavigationView.changeContent(fixedCore->getJView(), !first, enter);
        }

        updateChildren();
        reLayout();
    }

    bool NavigationViewCore::handleBackButton()
    {
        if (_stack.size() > 1) {
            popView();
            return true;
        }
        return false;
    }

    void NavigationViewCore::reLayout()
    {
        if (!_stack.empty()) {
            _stack.back().container->geometry = Rect{0, 0, geometry->width, geometry->height};
        }
    }
}
