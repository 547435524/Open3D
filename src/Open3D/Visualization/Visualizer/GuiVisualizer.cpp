// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "GuiVisualizer.h"

#include "Open3D/GUI/Application.h"
#include "Open3D/GUI/Button.h"
#include "Open3D/GUI/Checkbox.h"
#include "Open3D/GUI/Color.h"
#include "Open3D/GUI/ColorEdit.h"
#include "Open3D/GUI/Dialog.h"
#include "Open3D/GUI/FileDialog.h"
#include "Open3D/GUI/Label.h"
#include "Open3D/GUI/Layout.h"
#include "Open3D/GUI/SceneWidget.h"
#include "Open3D/GUI/Slider.h"
#include "Open3D/GUI/Theme.h"
#include "Open3D/Geometry/BoundingVolume.h"
#include "Open3D/Geometry/PointCloud.h"
#include "Open3D/Geometry/TriangleMesh.h"
#include "Open3D/IO/ClassIO/PointCloudIO.h"
#include "Open3D/IO/ClassIO/TriangleMeshIO.h"
#include "Open3D/Open3DConfig.h"
#include "Open3D/Utility/Console.h"
#include "Open3D/Utility/FileSystem.h"
#include "Open3D/Visualization/Rendering/RendererStructs.h"
#include "Open3D/Visualization/Rendering/Scene.h"

#include <array>

#define LOAD_IN_NEW_WINDOW 0

namespace open3d {
namespace visualization {

namespace {

std::shared_ptr<gui::Dialog> createAboutDialog(gui::Window *window) {
    auto &theme = window->GetTheme();
    auto dlg = std::make_shared<gui::Dialog>("About");

    auto title = std::make_shared<gui::Label>(
            (std::string("Open3D ") + OPEN3D_VERSION).c_str());
    auto text = std::make_shared<gui::Label>(
            "The MIT License (MIT)\n"
            "Copyright (c) 2018 www.open3d.org\n\n"

            "Permission is hereby granted, free of charge, to any person "
            "obtaining "
            "a copy of this software and associated documentation files (the "
            "\"Software\"), to deal in the Software without restriction, "
            "including "
            "without limitation the rights to use, copy, modify, merge, "
            "publish, "
            "distribute, sublicense, and/or sell copies of the Software, and "
            "to "
            "permit persons to whom the Software is furnished to do so, "
            "subject to "
            "the following conditions:\n\n"

            "The above copyright notice and this permission notice shall be "
            "included in all copies or substantial portions of the "
            "Software.\n\n"

            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, "
            "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES "
            "OF "
            "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND "
            "NONINFRINGEMENT. "
            "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR "
            "ANY "
            "CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF "
            "CONTRACT, "
            "TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE "
            "SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.");
    auto ok = std::make_shared<gui::Button>("OK");
    ok->SetOnClicked([window]() { window->CloseDialog(); });

    gui::Margins margins(theme.fontSize);
    auto layout = std::make_shared<gui::Vert>(0, margins);
    layout->AddChild(gui::Horiz::MakeCentered(title));
    layout->AddChild(gui::Horiz::MakeFixed(theme.fontSize));
    layout->AddChild(text);
    layout->AddChild(gui::Horiz::MakeFixed(theme.fontSize));
    layout->AddChild(gui::Horiz::MakeCentered(ok));
    dlg->AddChild(layout);

    return dlg;
}

std::shared_ptr<gui::Dialog> createContactDialog(gui::Window *window) {
    auto &theme = window->GetTheme();
    auto em = theme.fontSize;
    auto dlg = std::make_shared<gui::Dialog>("Contact Us");

    auto title = std::make_shared<gui::Label>("Contact Us");
    auto leftCol = std::make_shared<gui::Label>(
            "Web site:\n"
            "Code:\n"
            "Mailing list:\n"
            "Discord channel:");
    auto rightCol = std::make_shared<gui::Label>(
            "http://www.open3d.org\n"
            "http://github.org/intel-isl/Open3D\n"
            "http://www.open3d.org/index.php/subscribe/\n"
            "https://discord.gg/D35BGvn");
    auto ok = std::make_shared<gui::Button>("OK");
    ok->SetOnClicked([window]() { window->CloseDialog(); });

    gui::Margins margins(em);
    auto layout = std::make_shared<gui::Vert>(0, margins);
    layout->AddChild(gui::Horiz::MakeCentered(title));
    layout->AddChild(gui::Horiz::MakeFixed(em));

    auto columns = std::make_shared<gui::Horiz>(em, gui::Margins());
    columns->AddChild(leftCol);
    columns->AddChild(rightCol);
    layout->AddChild(columns);

    layout->AddChild(gui::Horiz::MakeFixed(em));
    layout->AddChild(gui::Horiz::MakeCentered(ok));
    dlg->AddChild(layout);

    return dlg;
}

std::shared_ptr<gui::Slider> MakeSlider(const gui::Slider::Type type,
                                        const double min,
                                        const double max,
                                        const double value) {
    auto slider = std::make_shared<gui::Slider>(type);
    slider->SetLimits(min, max);
    slider->SetValue(value);
    return slider;
}

class DrawTimeLabel : public gui::Label {
    using Super = Label;

public:
    DrawTimeLabel(gui::Window *w) : Label("0.0 ms") { window_ = w; }

    gui::Size CalcPreferredSize(const gui::Theme &theme) const override {
        auto h = Super::CalcPreferredSize(theme).height;
        return gui::Size(theme.fontSize * 5, h);
    }

    DrawResult Draw(const gui::DrawContext &context) override {
        char text[64];
        double ms = window_->GetLastFrameTimeSeconds() * 1000.0;
        snprintf(text, sizeof(text) - 1, "%.1f ms", ms);
        SetText(text);

        return Super::Draw(context);
    }

private:
    gui::Window *window_;
};

class SmallButton : public gui::Button {
    using Super = Button;

public:
    explicit SmallButton(const char *title) : Button(title) {}

    gui::Size CalcPreferredSize(const gui::Theme &theme) const override {
        auto em = theme.fontSize;
        auto size = Super::CalcPreferredSize(theme);
        return gui::Size(size.width - em, em);
    }
};

}  // namespace

static const float DEFAULT_SUN_INTENSITY = 100000.0f;
static const float DEFAULT_AMBIENT_INTENSITY = 50000.0f;
static const float DEFAULT_POINT_INTENSITY = 50000.0f;

enum MenuId {
    FILE_OPEN,
    FILE_EXPORT_RGB,
    FILE_EXPORT_DEPTH,
    FILE_CLOSE,
    VIEW_POINTS,
    VIEW_WIREFRAME,
    VIEW_MESH,
    SETTINGS_LIGHT,
    HELP_ABOUT,
    HELP_CONTACT
};

struct LightSettings {
    visualization::LightHandle hLight = visualization::LightHandle::kBad;
    Eigen::Vector3f position;

    std::shared_ptr<gui::Checkbox> wgtEnabled; // may be nullptr

    std::shared_ptr<gui::Slider> wgtIntensity;
    std::shared_ptr<gui::Button> wgtDirMinusX;
    std::shared_ptr<gui::Button> wgtDirPlusX;
    std::shared_ptr<gui::Button> wgtDirMinusY;
    std::shared_ptr<gui::Button> wgtDirPlusY;
    std::shared_ptr<gui::Button> wgtDirMinusZ;
    std::shared_ptr<gui::Button> wgtDirPlusZ;
    std::shared_ptr<gui::ColorEdit> wgtColor;

    std::shared_ptr<gui::VGrid> MakeLightUI(const gui::Theme& theme,
                                            Scene *scene);
    void SetEnabled(bool isEnabled) {
        this->wgtIntensity->SetEnabled(isEnabled);
        this->wgtDirMinusX->SetEnabled(isEnabled);
        this->wgtDirPlusX->SetEnabled(isEnabled);
        this->wgtDirMinusY->SetEnabled(isEnabled);
        this->wgtDirPlusY->SetEnabled(isEnabled);
        this->wgtDirMinusZ->SetEnabled(isEnabled);
        this->wgtDirPlusZ->SetEnabled(isEnabled);
        this->wgtColor->SetEnabled(isEnabled);
    }
};

struct GuiVisualizer::Impl {
    std::vector<visualization::GeometryHandle> geometryHandles;

    std::shared_ptr<gui::SceneWidget> scene;
    std::shared_ptr<gui::Horiz> drawTime;

    struct SceneSettings {
        visualization::IndirectLightHandle hIbl;
        visualization::SkyboxHandle hSky;

        std::shared_ptr<gui::Widget> wgtBase;
        std::shared_ptr<gui::Button> wgtLoadAmbient;
        std::shared_ptr<gui::Button> wgtLoadSky;
        std::shared_ptr<gui::Checkbox> wgtAmbientEnabled;
        std::shared_ptr<gui::Checkbox> wgtSkyEnabled;
        std::shared_ptr<gui::Checkbox> wgtDirectionalEnabled;
        std::shared_ptr<gui::Slider> wgtAmbientIntensity;
        LightSettings sun;
        std::array<LightSettings, 6> spotlights;
    } lightSettings;
};

GuiVisualizer::GuiVisualizer(
        const std::vector<std::shared_ptr<const geometry::Geometry>>
                &geometries,
        const std::string &title,
        int width,
        int height,
        int left,
        int top)
    : gui::Window(title, left, top, width, height),
      impl_(new GuiVisualizer::Impl()) {
    auto &app = gui::Application::GetInstance();
    auto &theme = GetTheme();

    // Create menu
    if (!gui::Application::GetInstance().GetMenubar()) {
        auto fileMenu = std::make_shared<gui::Menu>();
        fileMenu->AddItem("Open...", "Ctrl-O", FILE_OPEN);
        fileMenu->AddItem("Export RGB...", nullptr, FILE_EXPORT_RGB);
        fileMenu->SetEnabled(FILE_EXPORT_RGB, false);
        fileMenu->AddItem("Export depth image...", nullptr, FILE_EXPORT_DEPTH);
        fileMenu->SetEnabled(FILE_EXPORT_DEPTH, false);
        fileMenu->AddSeparator();
        fileMenu->AddItem("Close", "Ctrl-W", FILE_CLOSE);
        auto viewMenu = std::make_shared<gui::Menu>();
        viewMenu->AddItem("Points", nullptr, VIEW_POINTS);
        viewMenu->SetEnabled(VIEW_POINTS, false);
        viewMenu->AddItem("Wireframe", nullptr, VIEW_WIREFRAME);
        viewMenu->SetEnabled(VIEW_WIREFRAME, false);
        viewMenu->AddItem("Mesh", nullptr, VIEW_MESH);
        viewMenu->SetEnabled(VIEW_MESH, false);
        auto helpMenu = std::make_shared<gui::Menu>();
        helpMenu->AddItem("About", nullptr, HELP_ABOUT);
        helpMenu->AddItem("Contact", nullptr, HELP_CONTACT);
        auto settingsMenu = std::make_shared<gui::Menu>();
        settingsMenu->AddItem("Light", nullptr, SETTINGS_LIGHT);
        auto menu = std::make_shared<gui::Menu>();
        menu->AddMenu("File", fileMenu);
        menu->AddMenu("View", viewMenu);
        menu->AddMenu("Settings", settingsMenu);
        menu->AddMenu("Help", helpMenu);
        gui::Application::GetInstance().SetMenubar(menu);
    }

    // Create scene
    auto sceneId = GetRenderer().CreateScene();
    auto scene = std::make_shared<gui::SceneWidget>(
            *GetRenderer().GetScene(sceneId));
    impl_->scene = scene;
    scene->SetBackgroundColor(gui::Color(1.0, 1.0, 1.0));

    // Create sun
    visualization::LightDescription lightDescription;
    lightDescription.intensity = DEFAULT_SUN_INTENSITY;
    lightDescription.direction = {0.0, 0.0, -1.0};
    lightDescription.customAttributes["custom_type"] = "SUN";
    impl_->lightSettings.sun.hLight =
            scene->GetScene()->AddLight(lightDescription);
    impl_->lightSettings.sun.position = lightDescription.direction;

    // Create ambient light (IBL)
    auto &lightSettings = impl_->lightSettings;
    std::string rsrcPath = app.GetResourcePath();
    auto iblPath = rsrcPath + "/default_ibl.ktx";
    lightSettings.hIbl =
            GetRenderer().AddIndirectLight(ResourceLoadRequest(iblPath.data()));
    scene->GetScene()->SetIndirectLight(lightSettings.hIbl);
    const auto kAmbientIntensity = DEFAULT_AMBIENT_INTENSITY;
    scene->GetScene()->SetIndirectLightIntensity(kAmbientIntensity);

    auto skyPath = rsrcPath + "/default_sky.ktx";
    lightSettings.hSky =
            GetRenderer().AddSkybox(ResourceLoadRequest(skyPath.data()));

    // Config spotlights
    const float infinity = 500.0f;
    impl_->lightSettings.spotlights[0].position = {-infinity, 0.0f, 0.0f};
    impl_->lightSettings.spotlights[1].position = {infinity, 0.0f, 0.0f};
    impl_->lightSettings.spotlights[2].position = {0.0f, -infinity, 0.0f};
    impl_->lightSettings.spotlights[3].position = {0.0f, infinity, 0.0f};
    impl_->lightSettings.spotlights[4].position = {0.0f, 0.0f, -infinity};
    impl_->lightSettings.spotlights[5].position = {0.0f, 0.0f, infinity};

    // Add the geometry
    SetGeometry(geometries);  // also updates the camera

    // Setup UI
    const auto em = theme.fontSize;
    int spacing = std::max(1, int(std::ceil(0.25 * em)));

    auto drawTimeLabel = std::make_shared<DrawTimeLabel>(this);
    drawTimeLabel->SetTextColor(gui::Color(0.5, 0.5, 0.5));
    impl_->drawTime = std::make_shared<gui::Horiz>(0, gui::Margins(spacing, 0));
    impl_->drawTime->SetBackgroundColor(gui::Color(0, 0, 0, 0));
    impl_->drawTime->AddChild(drawTimeLabel);

    AddChild(scene);

    auto renderScene = scene->GetScene();

    // Add light settings widget
    const int separationHeight = std::ceil(em);
    const int lm = std::ceil(0.5 * em);
    const int gridSpacing = std::ceil(0.25 * em);
    lightSettings.wgtBase = std::make_shared<gui::Vert>(0, gui::Margins(lm));

    lightSettings.wgtLoadAmbient = std::make_shared<gui::Button>("Load IBL");
    lightSettings.wgtLoadAmbient->SetOnClicked([this, renderScene]() {
        auto dlg = std::make_shared<gui::FileDialog>(
                gui::FileDialog::Type::OPEN, "Open IBL", GetTheme());
        dlg->AddFilter(".ktx", "Khronos Texture (.ktx)");
        dlg->SetOnCancel([this]() { this->CloseDialog(); });
        dlg->SetOnDone([this, renderScene](const char *path) {
            this->CloseDialog();
            auto newIBL =
                    GetRenderer().AddIndirectLight(ResourceLoadRequest(path));
            if (newIBL) {
                impl_->lightSettings.hIbl = newIBL;

                auto intensity = renderScene->GetIndirectLightIntensity();

                renderScene->SetIndirectLight(newIBL);
                renderScene->SetIndirectLightIntensity(intensity);
            }
        });
        ShowDialog(dlg);
    });

    lightSettings.wgtLoadSky = std::make_shared<gui::Button>("Load skybox");
    lightSettings.wgtLoadSky->SetOnClicked([this, renderScene]() {
        auto dlg = std::make_shared<gui::FileDialog>(
                gui::FileDialog::Type::OPEN, "Open skybox", GetTheme());
        dlg->AddFilter(".ktx", "Khronos Texture (.ktx)");
        dlg->SetOnCancel([this]() { this->CloseDialog(); });
        dlg->SetOnDone([this, renderScene](const char *path) {
            this->CloseDialog();
            auto newSky = GetRenderer().AddSkybox(ResourceLoadRequest(path));
            if (newSky) {
                impl_->lightSettings.hSky = newSky;
                impl_->lightSettings.wgtSkyEnabled->SetChecked(true);

                renderScene->SetSkybox(newSky);
            }
        });
        ShowDialog(dlg);
    });

    auto loadButtons = std::make_shared<gui::Horiz>(spacing, gui::Margins(0));
    loadButtons->AddChild(gui::Horiz::MakeStretch());
    loadButtons->AddChild(lightSettings.wgtLoadAmbient);
    loadButtons->AddChild(lightSettings.wgtLoadSky);
    loadButtons->AddChild(gui::Horiz::MakeStretch());
    lightSettings.wgtBase->AddChild(loadButtons);

    lightSettings.wgtBase->AddChild(gui::Horiz::MakeFixed(separationHeight));

    // ... lighting on/off
    lightSettings.wgtBase->AddChild(
            std::make_shared<gui::Label>("> Light sources"));
    auto checkboxes = std::make_shared<gui::Horiz>();
    lightSettings.wgtAmbientEnabled =
            std::make_shared<gui::Checkbox>("Ambient");
    lightSettings.wgtAmbientEnabled->SetChecked(true);
    lightSettings.wgtAmbientEnabled->SetOnChecked(
            [this, renderScene](bool checked) {
                if (checked) {
                    renderScene->SetIndirectLight(impl_->lightSettings.hIbl);
                } else {
                    renderScene->SetIndirectLight(IndirectLightHandle());
                }
            });
    checkboxes->AddChild(lightSettings.wgtAmbientEnabled);
    lightSettings.wgtSkyEnabled = std::make_shared<gui::Checkbox>("Sky");
    lightSettings.wgtSkyEnabled->SetChecked(false);
    lightSettings.wgtSkyEnabled->SetOnChecked(
            [this, renderScene](bool checked) {
                if (checked) {
                    renderScene->SetSkybox(impl_->lightSettings.hSky);
                } else {
                    renderScene->SetSkybox(SkyboxHandle());
                }
            });
    checkboxes->AddChild(lightSettings.wgtSkyEnabled);
    lightSettings.wgtDirectionalEnabled =
            std::make_shared<gui::Checkbox>("Sun");
    lightSettings.wgtDirectionalEnabled->SetChecked(true);
    lightSettings.wgtDirectionalEnabled->SetOnChecked(
            [this, renderScene](bool checked) {
                renderScene->SetEntityEnabled(
                        impl_->lightSettings.sun.hLight, checked);
            });
    checkboxes->AddChild(lightSettings.wgtDirectionalEnabled);
    lightSettings.wgtBase->AddChild(checkboxes);

    lightSettings.wgtBase->AddChild(gui::Horiz::MakeFixed(separationHeight));

    // ... ambient light (IBL)
    lightSettings.wgtAmbientIntensity =
            MakeSlider(gui::Slider::INT, 0.0, 150000.0, kAmbientIntensity);
    lightSettings.wgtAmbientIntensity->OnValueChanged =
            [renderScene](double newValue) {
                renderScene->SetIndirectLightIntensity(newValue);
            };

    auto ambientLayout = std::make_shared<gui::VGrid>(2, gridSpacing);
    ambientLayout->AddChild(std::make_shared<gui::Label>("Intensity"));
    ambientLayout->AddChild(lightSettings.wgtAmbientIntensity);

    lightSettings.wgtBase->AddChild(std::make_shared<gui::Label>("> Ambient"));
    lightSettings.wgtBase->AddChild(ambientLayout);
    lightSettings.wgtBase->AddChild(gui::Horiz::MakeFixed(separationHeight));

    // ... directional light (sun)
    lightSettings.wgtBase->AddChild(
            std::make_shared<gui::Label>("> Sun (Directional light)"));
    lightSettings.wgtBase->AddChild(lightSettings.sun.MakeLightUI(
                                                        theme, renderScene));

    // ... spotlights
    lightSettings.wgtBase->AddChild(gui::Horiz::MakeFixed(separationHeight));
    lightSettings.wgtBase->AddChild(
            std::make_shared<gui::Label>("> Spotlight -X"));
    lightSettings.wgtBase->AddChild(lightSettings.spotlights[0].MakeLightUI(
                                                        theme, renderScene));
    lightSettings.wgtBase->AddChild(gui::Horiz::MakeFixed(separationHeight));
    lightSettings.wgtBase->AddChild(
            std::make_shared<gui::Label>("> Spotlight +X"));
    lightSettings.wgtBase->AddChild(lightSettings.spotlights[1].MakeLightUI(
                                                        theme, renderScene));
    lightSettings.wgtBase->AddChild(gui::Horiz::MakeFixed(separationHeight));
    lightSettings.wgtBase->AddChild(
            std::make_shared<gui::Label>("> Spotlight -Y"));
    lightSettings.wgtBase->AddChild(lightSettings.spotlights[2].MakeLightUI(
                                                        theme, renderScene));
    lightSettings.wgtBase->AddChild(gui::Horiz::MakeFixed(separationHeight));
    lightSettings.wgtBase->AddChild(
            std::make_shared<gui::Label>("> Spotlight +Y"));
    lightSettings.wgtBase->AddChild(lightSettings.spotlights[3].MakeLightUI(
                                                        theme, renderScene));
    lightSettings.wgtBase->AddChild(gui::Horiz::MakeFixed(separationHeight));
    lightSettings.wgtBase->AddChild(
            std::make_shared<gui::Label>("> Spotlight -Z"));
    lightSettings.wgtBase->AddChild(lightSettings.spotlights[4].MakeLightUI(
                                                        theme, renderScene));
    lightSettings.wgtBase->AddChild(gui::Horiz::MakeFixed(separationHeight));
    lightSettings.wgtBase->AddChild(
            std::make_shared<gui::Label>("> Spotlight +Z"));
    lightSettings.wgtBase->AddChild(lightSettings.spotlights[5].MakeLightUI(
                                                        theme, renderScene));

    AddChild(lightSettings.wgtBase);

    lightSettings.wgtBase->SetVisible(false);

    AddChild(impl_->drawTime);
}

std::shared_ptr<gui::VGrid> LightSettings::MakeLightUI(
                                                const gui::Theme& theme,
                                                Scene *scene) {
    const int gridSpacing = std::ceil(0.25 * theme.fontSize);

    bool isSun = false;
    float intensity = DEFAULT_POINT_INTENSITY;
    if (this->hLight != visualization::LightHandle::kBad) {
        intensity = scene->GetLightIntensity(this->hLight);
        isSun = true;
    }
    if (!isSun) {
        this->wgtEnabled = std::make_shared<gui::Checkbox>("    ");
        this->wgtEnabled->SetOnChecked([scene, this](bool checked){
            if (checked) {
                visualization::LightDescription lightDescription;
                lightDescription.type = visualization::LightDescription::POINT;
                lightDescription.intensity =
                                    float(this->wgtIntensity->GetIntValue());
                lightDescription.falloff = 10000.0f;
                lightDescription.lightConeInner = 90.0f;
                lightDescription.lightConeOuter = 90.0f;
                lightDescription.direction = -this->position.normalized();
                lightDescription.castShadows = false;
                auto& color = this->wgtColor->GetValue();
                lightDescription.color = {color.GetRed(), color.GetGreen(),
                                          color.GetBlue()};
                this->hLight = scene->AddLight(lightDescription);
                scene->SetLightPosition(this->hLight, this->position);
            } else {
                scene->RemoveLight(this->hLight);
            }
            this->SetEnabled(checked);
        });
    }
    this->wgtIntensity = MakeSlider(gui::Slider::INT, 0.0, 500000.0, intensity);
    this->wgtIntensity->OnValueChanged =
            [scene, this](double newValue) {
                scene->SetLightIntensity(this->hLight, newValue);
            };

    this->wgtDirMinusX = std::make_shared<SmallButton>("-X");
    this->wgtDirMinusX->SetOnClicked([scene, this]() {
        scene->SetLightDirection(this->hLight, {1.0f, 0.0f, 0.0f});
    });
    this->wgtDirPlusX = std::make_shared<SmallButton>("+X");
    this->wgtDirPlusX->SetOnClicked([scene, this]() {
        scene->SetLightDirection(this->hLight, {-1.0f, 0.0f, 0.0f});
    });
    this->wgtDirMinusY = std::make_shared<SmallButton>("-Y");
    this->wgtDirMinusY->SetOnClicked([scene, this]() {
        scene->SetLightDirection(this->hLight, {0.0f, 1.0f, 0.0f});
    });
    this->wgtDirPlusY = std::make_shared<SmallButton>("+Y");
    this->wgtDirPlusY->SetOnClicked([scene, this]() {
        scene->SetLightDirection(this->hLight, {0.0f, -1.0f, 0.0f});
    });
    this->wgtDirMinusZ = std::make_shared<SmallButton>("-Z");
    this->wgtDirMinusZ->SetOnClicked([scene, this]() {
        scene->SetLightDirection(this->hLight, {0.0f, 0.0f, 1.0f});
    });
    this->wgtDirPlusZ = std::make_shared<SmallButton>("+Z");
    this->wgtDirPlusZ->SetOnClicked([scene, this]() {
        scene->SetLightDirection(this->hLight, {0.0f, 0.0f, -1.0f});
    });
    auto sunDirLayout = std::make_shared<gui::Horiz>(gridSpacing);
    sunDirLayout->AddChild(this->wgtDirMinusX);
    sunDirLayout->AddChild(gui::Horiz::MakeStretch());
    sunDirLayout->AddChild(this->wgtDirPlusX);
    sunDirLayout->AddChild(gui::Horiz::MakeStretch());
    sunDirLayout->AddChild(this->wgtDirMinusY);
    sunDirLayout->AddChild(gui::Horiz::MakeStretch());
    sunDirLayout->AddChild(this->wgtDirPlusY);
    sunDirLayout->AddChild(gui::Horiz::MakeStretch());
    sunDirLayout->AddChild(this->wgtDirMinusZ);
    sunDirLayout->AddChild(gui::Horiz::MakeStretch());
    sunDirLayout->AddChild(this->wgtDirPlusZ);

    this->wgtColor = std::make_shared<gui::ColorEdit>();
    this->wgtColor->SetValue({1, 1, 1});
    this->wgtColor->OnValueChanged =
            [scene, this](const gui::Color &newColor) {
                scene->SetLightColor(this->hLight,
                        {newColor.GetRed(), newColor.GetGreen(),
                         newColor.GetBlue()});
            };

    auto sunLayout = std::make_shared<gui::VGrid>(2, gridSpacing);
    if (!isSun) {
        sunLayout->AddChild(std::make_shared<gui::Label>("Enabled"));
        sunLayout->AddChild(this->wgtEnabled);
    }
    sunLayout->AddChild(std::make_shared<gui::Label>("Intensity"));
    sunLayout->AddChild(this->wgtIntensity);
    if (isSun) {
        sunLayout->AddChild(std::make_shared<gui::Label>("Position"));
        sunLayout->AddChild(sunDirLayout);
    }
    sunLayout->AddChild(std::make_shared<gui::Label>("Color"));
    sunLayout->AddChild(this->wgtColor);

    SetEnabled(isSun);

    return sunLayout;
}

GuiVisualizer::~GuiVisualizer() {}

void GuiVisualizer::SetTitle(const std::string &title) {
    Super::SetTitle(title.c_str());
}

void GuiVisualizer::SetGeometry(
        const std::vector<std::shared_ptr<const geometry::Geometry>>
                &geometries) {
    auto *scene3d = impl_->scene->GetScene();
    for (auto &h : impl_->geometryHandles) {
        scene3d->RemoveGeometry(h);
    }
    impl_->geometryHandles.clear();

    geometry::AxisAlignedBoundingBox bounds;
    for (auto &g : geometries) {
        switch (g->GetGeometryType()) {
            case geometry::Geometry::GeometryType::OrientedBoundingBox:
            case geometry::Geometry::GeometryType::AxisAlignedBoundingBox:
            case geometry::Geometry::GeometryType::PointCloud:
            case geometry::Geometry::GeometryType::LineSet:
            case geometry::Geometry::GeometryType::MeshBase:
            case geometry::Geometry::GeometryType::TriangleMesh:
            case geometry::Geometry::GeometryType::HalfEdgeTriangleMesh:
            case geometry::Geometry::GeometryType::TetraMesh:
            case geometry::Geometry::GeometryType::Octree:
            case geometry::Geometry::GeometryType::VoxelGrid: {
                auto g3 =
                        std::static_pointer_cast<const geometry::Geometry3D>(g);
                bounds += g3->GetAxisAlignedBoundingBox();
                auto handle = scene3d->AddGeometry(*g3);

                impl_->geometryHandles.push_back(handle);
            }
            case geometry::Geometry::GeometryType::RGBDImage:
            case geometry::Geometry::GeometryType::Image:
            case geometry::Geometry::GeometryType::Unspecified:
                break;
        }
    }

    impl_->scene->SetupCamera(60.0, bounds, bounds.GetCenter().cast<float>());
}

void GuiVisualizer::Layout(const gui::Theme &theme) {
    auto r = GetContentRect();
    const auto em = theme.fontSize;
    impl_->scene->SetFrame(r);

    const auto pref = impl_->drawTime->CalcPreferredSize(theme);
    impl_->drawTime->SetFrame(
            gui::Rect(0, r.GetBottom() - pref.height, 5 * em, pref.height));
    impl_->drawTime->Layout(theme);

    const auto kLightSettingsWidth = 18 * em;
    auto lightSettingsSize =
            impl_->lightSettings.wgtBase->CalcPreferredSize(theme);
    gui::Rect lightSettingsRect(r.width - kLightSettingsWidth, r.y,
                                kLightSettingsWidth, lightSettingsSize.height);
    impl_->lightSettings.wgtBase->SetFrame(lightSettingsRect);

    Super::Layout(theme);
}

bool GuiVisualizer::LoadGeometry(const std::string &path) {
    auto geometry = std::shared_ptr<geometry::Geometry3D>();

    auto mesh = std::make_shared<geometry::TriangleMesh>();
    bool meshSuccess = false;
    try {
        meshSuccess = io::ReadTriangleMesh(path, *mesh);
    } catch (...) {
        meshSuccess = false;
    }
    if (meshSuccess) {
        if (mesh->triangles_.size() == 0) {
            utility::LogWarning(
                    "Contains 0 triangles, will read as point cloud");
            mesh.reset();
        } else {
            mesh->ComputeVertexNormals();
            geometry = mesh;
        }
    } else {
        // LogError throws an exception, which we don't want, because this might
        // be a point cloud.
        utility::LogWarning("Failed to read %s", path.c_str());
        mesh.reset();
    }

    if (!geometry) {
        auto cloud = std::make_shared<geometry::PointCloud>();
        bool success = false;
        try {
            success = io::ReadPointCloud(path, *cloud);
        } catch (...) {
            success = false;
        }
        if (success) {
            utility::LogInfof("Successfully read %s", path.c_str());
            cloud->NormalizeNormals();
            geometry = cloud;
        } else {
            utility::LogWarning("Failed to read points %s", path.c_str());
            cloud.reset();
        }
    }

    if (geometry) {
        SetGeometry({geometry});
    }
    return (geometry != nullptr);
}

void GuiVisualizer::ExportRGB(const std::string &path) {
    ShowMessageBox("Not implemented", "ExportRGB() is not implemented yet");
}

void GuiVisualizer::ExportDepth(const std::string &path) {
    ShowMessageBox("Not implemented", "ExportDepth() is not implemented yet");
}

void GuiVisualizer::OnMenuItemSelected(gui::Menu::ItemId itemId) {
    auto menuId = MenuId(itemId);
    switch (menuId) {
        case FILE_OPEN: {
            auto dlg = std::make_shared<gui::FileDialog>(
                    gui::FileDialog::Type::OPEN, "Open Geometry", GetTheme());
            dlg->AddFilter(".ply .stl .obj .off .gltf .glb",
                           "Triangle mesh files (.ply, .stl, .obj, .off, "
                           ".gltf, .glb)");
            dlg->AddFilter(".xyz .xyzn .xyzrgb .ply .pcd .pts",
                           "Point cloud files (.xyz, .xyzn, .xyzrgb, .ply, "
                           ".pcd, .pts)");
            dlg->AddFilter(".ply", "Polygon files (.ply)");
            dlg->AddFilter(".stl", "Stereolithography files (.stl)");
            dlg->AddFilter(".obj", "Wavefront OBJ files (.obj)");
            dlg->AddFilter(".off", "Object file format (.off)");
            dlg->AddFilter(".gltf", "OpenGL transfer files (.gltf)");
            dlg->AddFilter(".glb", "OpenGL binary transfer files (.glb)");
            dlg->AddFilter(".xyz", "ASCII point cloud files (.xyz)");
            dlg->AddFilter(".xyzn", "ASCII point cloud with normals (.xyzn)");
            dlg->AddFilter(".xyzrgb",
                           "ASCII point cloud files with colors (.xyzrgb)");
            dlg->AddFilter(".pcd", "Point Cloud Data files (.pcd)");
            dlg->AddFilter(".pts", "3D Points files (.pts)");
            dlg->AddFilter("", "All files");
            dlg->SetOnCancel([this]() { this->CloseDialog(); });
            dlg->SetOnDone([this](const char *path) {
                this->CloseDialog();
                OnDragDropped(path);
            });
            ShowDialog(dlg);
            break;
        }
        case FILE_EXPORT_RGB:  // fall through
        case FILE_EXPORT_DEPTH: {
            auto dlg = std::make_shared<gui::FileDialog>(
                    gui::FileDialog::Type::SAVE, "Save File", GetTheme());
            dlg->AddFilter(".png", "PNG images (.png)");
            dlg->AddFilter("", "All files");
            dlg->SetOnCancel([this]() { this->CloseDialog(); });
            dlg->SetOnDone([this, menuId](const char *path) {
                this->CloseDialog();
                if (menuId == FILE_EXPORT_RGB) {
                    this->ExportRGB(path);
                } else {
                    this->ExportDepth(path);
                }
            });
            ShowDialog(dlg);
            break;
        }
        case FILE_CLOSE:
            this->Close();
            break;
        case VIEW_POINTS:
            break;
        case VIEW_WIREFRAME:
            break;
        case VIEW_MESH:
            break;
        case SETTINGS_LIGHT: {
            auto visibility = !impl_->lightSettings.wgtBase->IsVisible();
            impl_->lightSettings.wgtBase->SetVisible(visibility);
            auto menubar = gui::Application::GetInstance().GetMenubar();
            menubar->SetChecked(SETTINGS_LIGHT, visibility);
            break;
        }
        case HELP_ABOUT: {
            auto dlg = createAboutDialog(this);
            ShowDialog(dlg);
            break;
        }
        case HELP_CONTACT: {
            auto dlg = createContactDialog(this);
            ShowDialog(dlg);
            break;
        }
    }
}

void GuiVisualizer::OnDragDropped(const char *path) {
    auto title = std::string("Open3D - ") + path;
#if LOAD_IN_NEW_WINDOW
    auto frame = this->GetFrame();
    std::vector<std::shared_ptr<const geometry::Geometry>> nothing;
    auto vis = std::make_shared<GuiVisualizer>(nothing, title.c_str(),
                                               frame.width, frame.height,
                                               frame.x + 20, frame.y + 20);
    gui::Application::GetInstance().AddWindow(vis);
#else
    this->SetTitle(title);
    auto vis = this;
#endif  // LOAD_IN_NEW_WINDOW
    if (!vis->LoadGeometry(path)) {
        auto err = std::string("Error reading geometry file '") + path + "'";
        vis->ShowMessageBox("Error loading geometry", err.c_str());
    }
}

}  // namespace visualization
}  // namespace open3d
