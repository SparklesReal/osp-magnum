/**
 * Open Space Program
 * Copyright © 2019-2021 Open Space Program Project
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

#include <osp/Active/ActiveScene.h>
#include <osp/Resource/PackageRegistry.h>

#include <osp/types.h>
#include <osp/UserInputHandler.h>

#include <Magnum/Timeline.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <cstring> // workaround: memcpy needed by SDL2
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/VertexColorGL.h>

#include <memory>

namespace testapp
{

using SceneUpdate_t = std::function<void(osp::active::ActiveScene&)>;

using MapActiveScene_t = std::map<
        std::string,
        std::pair<osp::active::ActiveScene, SceneUpdate_t>,
        std::less<> >;

/**
 * @brief An interactive Magnum application made for running ActiveScenes
 *
 * These scenes can be a flight scene, map view, vehicle editor, or menu.
 */
class ActiveApplication : public Magnum::Platform::Application
{

public:

    using on_draw_t = std::function<void(ActiveApplication&)>;

    explicit ActiveApplication(
            const Magnum::Platform::Application::Arguments& arguments,
            osp::PackageRegistry &rPkgs,
            on_draw_t onDraw);

    ~ActiveApplication();

    void keyPressEvent(KeyEvent& event) override;
    void keyReleaseEvent(KeyEvent& event) override;

    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void mouseScrollEvent(MouseScrollEvent& event) override;

    void update_scenes();
    void draw_scenes();

    osp::active::ActiveScene& scene_create(std::string const& name, SceneUpdate_t upd);
    osp::active::ActiveScene& scene_create(std::string&& name, SceneUpdate_t upd);

    constexpr osp::input::UserInputHandler& get_input_handler() { return m_userInput; }
    constexpr MapActiveScene_t& get_scenes() { return m_scenes; }

    constexpr osp::Package& get_context_resources() { return m_glResources; }

private:

    void drawEvent() override;

    on_draw_t m_onDraw;

    osp::input::UserInputHandler m_userInput;

    MapActiveScene_t m_scenes;

    osp::PackageRegistry &m_rPackages;

    osp::Package m_glResources;

    Magnum::Timeline m_timeline;
};

void config_controls(ActiveApplication& rPkgs);

}

/**
* Parses the control string from the config file. 
* 
* A "None" input returns a empty vector.
* 
* @param Control string
* @returns vector of the control created from the string. 
*/
osp::input::ControlExprConfig_t parse_control(std::string_view str) noexcept;