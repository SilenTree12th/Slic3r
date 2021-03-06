#include "libslic3r/libslic3r.h"
#include "libslic3r/GCode/PreviewData.hpp"
#include "GUI_Preview.hpp"
#include "GUI_App.hpp"
#include "GUI.hpp"
#include "AppConfig.hpp"
#include "3DScene.hpp"
#include "BackgroundSlicingProcess.hpp"
#include "GLCanvas3DManager.hpp"
#include "GLCanvas3D.hpp"
#include "PresetBundle.hpp"
#include "wxExtensions.hpp"

#include <wx/notebook.h>
#include <wx/glcanvas.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/combo.h>
#include <wx/checkbox.h>

// this include must follow the wxWidgets ones or it won't compile on Windows -> see http://trac.wxwidgets.org/ticket/2421
#include "../../libslic3r/Print.hpp"

namespace Slic3r {
namespace GUI {


Preview::Preview(wxNotebook* notebook, DynamicPrintConfig* config, BackgroundSlicingProcess* process, GCodePreviewData* gcode_preview_data, std::function<void()> schedule_background_process_func)
    : m_canvas_widget(nullptr)
    , m_canvas(nullptr)
    , m_double_slider_sizer(nullptr)
    , m_label_view_type(nullptr)
    , m_choice_view_type(nullptr)
    , m_label_show_features(nullptr)
    , m_combochecklist_features(nullptr)
    , m_checkbox_travel(nullptr)
    , m_checkbox_retractions(nullptr)
    , m_checkbox_unretractions(nullptr)
    , m_checkbox_shells(nullptr)
    , m_config(config)
    , m_process(process)
    , m_gcode_preview_data(gcode_preview_data)
    , m_number_extruders(1)
    , m_preferred_color_mode("feature")
    , m_loaded(false)
    , m_enabled(false)
    , m_force_sliders_full_range(false)
    , m_schedule_background_process(schedule_background_process_func)
{
    if (init(notebook, config, process, gcode_preview_data))
    {
        notebook->AddPage(this, _(L("Preview")));
        show_hide_ui_elements("none");
        load_print();
    }
}

bool Preview::init(wxNotebook* notebook, DynamicPrintConfig* config, BackgroundSlicingProcess* process, GCodePreviewData* gcode_preview_data)
{
    if ((notebook == nullptr) || (config == nullptr) || (process == nullptr) || (gcode_preview_data == nullptr))
        return false;

    // creates this panel add append it to the given notebook as a new page
    if (!Create(notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize))
        return false;

    m_canvas_widget = GLCanvas3DManager::create_wxglcanvas(this);
	_3DScene::add_canvas(m_canvas_widget);
	m_canvas = _3DScene::get_canvas(this->m_canvas_widget);
    m_canvas->allow_multisample(GLCanvas3DManager::can_multisample());
    m_canvas->enable_shader(true);
    m_canvas->set_config(m_config);
    m_canvas->set_process(process);
    m_canvas->enable_legend_texture(true);
    m_canvas->enable_dynamic_background(true);

    m_double_slider_sizer = new wxBoxSizer(wxHORIZONTAL);
    create_double_slider();

    m_label_view_type = new wxStaticText(this, wxID_ANY, _(L("View")));

    m_choice_view_type = new wxChoice(this, wxID_ANY);
    m_choice_view_type->Append(_(L("Feature type")));
    m_choice_view_type->Append(_(L("Height")));
    m_choice_view_type->Append(_(L("Width")));
    m_choice_view_type->Append(_(L("Speed")));
    m_choice_view_type->Append(_(L("Volumetric flow rate")));
    m_choice_view_type->Append(_(L("Tool")));
    m_choice_view_type->SetSelection(0);

    m_label_show_features = new wxStaticText(this, wxID_ANY, _(L("Show")));

    m_combochecklist_features = new wxComboCtrl();
    m_combochecklist_features->Create(this, wxID_ANY, _(L("Feature types")), wxDefaultPosition, wxSize(200, -1), wxCB_READONLY);
    std::string feature_text = GUI::into_u8(_(L("Feature types")));
    std::string feature_items = GUI::into_u8(
        _(L("Perimeter")) + "|" +
        _(L("External perimeter")) + "|" +
        _(L("Overhang perimeter")) + "|" +
        _(L("Internal infill")) + "|" +
        _(L("Solid infill")) + "|" +
        _(L("Top solid infill")) + "|" +
        _(L("Bridge infill")) + "|" +
        _(L("Gap fill")) + "|" +
        _(L("Skirt")) + "|" +
        _(L("Support material")) + "|" +
        _(L("Support material interface")) + "|" +
        _(L("Wipe tower")) + "|" +
        _(L("Custom"))
    );
    Slic3r::GUI::create_combochecklist(m_combochecklist_features, feature_text, feature_items, true);

    m_checkbox_travel = new wxCheckBox(this, wxID_ANY, _(L("Travel")));
    m_checkbox_retractions = new wxCheckBox(this, wxID_ANY, _(L("Retractions")));
    m_checkbox_unretractions = new wxCheckBox(this, wxID_ANY, _(L("Unretractions")));
    m_checkbox_shells = new wxCheckBox(this, wxID_ANY, _(L("Shells")));

    wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
    top_sizer->Add(m_canvas_widget, 1, wxALL | wxEXPAND, 0);
    top_sizer->Add(m_double_slider_sizer, 0, wxEXPAND, 0);

    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
    bottom_sizer->Add(m_label_view_type, 0, wxALIGN_CENTER_VERTICAL, 5);
    bottom_sizer->Add(m_choice_view_type, 0, wxEXPAND | wxALL, 5);
    bottom_sizer->AddSpacer(10);
    bottom_sizer->Add(m_label_show_features, 0, wxALIGN_CENTER_VERTICAL, 5);
    bottom_sizer->Add(m_combochecklist_features, 0, wxEXPAND | wxALL, 5);
    bottom_sizer->AddSpacer(20);
    bottom_sizer->Add(m_checkbox_travel, 0, wxEXPAND | wxALL, 5);
    bottom_sizer->AddSpacer(10);
    bottom_sizer->Add(m_checkbox_retractions, 0, wxEXPAND | wxALL, 5);
    bottom_sizer->AddSpacer(10);
    bottom_sizer->Add(m_checkbox_unretractions, 0, wxEXPAND | wxALL, 5);
    bottom_sizer->AddSpacer(10);
    bottom_sizer->Add(m_checkbox_shells, 0, wxEXPAND | wxALL, 5);

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(top_sizer, 1, wxALL | wxEXPAND, 0);
    main_sizer->Add(bottom_sizer, 0, wxALL | wxEXPAND, 0);

    SetSizer(main_sizer);
    SetMinSize(GetSize());
    GetSizer()->SetSizeHints(this);

    bind_event_handlers();
    
    // sets colors for gcode preview extrusion roles
    std::vector<std::string> extrusion_roles_colors = {
        "Perimeter", "FFFF66",
        "External perimeter", "FFA500",
        "Overhang perimeter", "0000FF",
        "Internal infill", "B1302A",
        "Solid infill", "D732D7",
        "Top solid infill", "FF1A1A",
        "Bridge infill", "9999FF",
        "Gap fill", "FFFFFF",
        "Skirt", "845321",
        "Support material", "00FF00",
        "Support material interface", "008000",
        "Wipe tower", "B3E3AB",
        "Custom", "28CC94"
    };
    m_gcode_preview_data->set_extrusion_paths_colors(extrusion_roles_colors);

    return true;
}

Preview::~Preview()
{
    unbind_event_handlers();

    if (m_canvas_widget != nullptr)
    {
		_3DScene::remove_canvas(m_canvas_widget);
        delete m_canvas_widget;
        m_canvas = nullptr;
    }
}

void Preview::set_number_extruders(unsigned int number_extruders)
{
    if (m_number_extruders != number_extruders)
    {
        m_number_extruders = number_extruders;
        int type = 0; // color by a feature type
        if (number_extruders > 1)
        {
            int tool_idx = m_choice_view_type->FindString(_(L("Tool")));
            int type = (number_extruders > 1) ? tool_idx /* color by a tool number */  : 0; // color by a feature type
            m_choice_view_type->SetSelection(type);
            if ((0 <= type) && (type < (int)GCodePreviewData::Extrusion::Num_View_Types))
                m_gcode_preview_data->extrusion.view_type = (GCodePreviewData::Extrusion::EViewType)type;

            m_preferred_color_mode = (type == tool_idx) ? "tool_or_feature" : "feature";
        }
    }
}

void Preview::reset_gcode_preview_data()
{
    m_gcode_preview_data->reset();
    m_canvas->reset_legend_texture();
}

void Preview::set_canvas_as_dirty()
{
    m_canvas->set_as_dirty();
}

void Preview::set_enabled(bool enabled)
{
    m_enabled = enabled;
}

void Preview::set_bed_shape(const Pointfs& shape)
{
    m_canvas->set_bed_shape(shape);
}

void Preview::select_view(const std::string& direction)
{
    m_canvas->select_view(direction);
}

void Preview::set_viewport_from_scene(GLCanvas3D* canvas)
{
    if (canvas != nullptr)
        m_canvas->set_viewport_from_scene(*canvas);
}

void Preview::set_viewport_into_scene(GLCanvas3D* canvas)
{
    if (canvas != nullptr)
		canvas->set_viewport_from_scene(*m_canvas);
}

void Preview::set_drop_target(wxDropTarget* target)
{
    if (target != nullptr)
        SetDropTarget(target);
}

void Preview::load_print()
{
	if (m_loaded || m_process->current_printer_technology() != ptFFF)
        return;

    // we require that there's at least one object and the posSlice step
    // is performed on all of them(this ensures that _shifted_copies was
    // populated and we know the number of layers)
    unsigned int n_layers = 0;
    const Print *print = m_process->fff_print();
    if (print->is_step_done(posSlice))
    {
        std::set<float> zs;
        for (const PrintObject* print_object : print->objects())
        {
            const LayerPtrs& layers = print_object->layers();
            const SupportLayerPtrs& support_layers = print_object->support_layers();
            for (const Layer* layer : layers)
            {
                zs.insert(layer->print_z);
            }
            for (const SupportLayer* layer : support_layers)
            {
                zs.insert(layer->print_z);
            }
        }

        n_layers = (unsigned int)zs.size();
    }

    if (n_layers == 0)
    {
        reset_sliders();
        m_canvas->reset_legend_texture();
        m_canvas_widget->Refresh();
        return;
    }

    if (m_preferred_color_mode == "tool_or_feature")
    {
        // It is left to Slic3r to decide whether the print shall be colored by the tool or by the feature.
        // Color by feature if it is a single extruder print.
        unsigned int number_extruders = (unsigned int)print->extruders().size();
        int tool_idx = m_choice_view_type->FindString(_(L("Tool")));
        int type = (number_extruders > 1) ? tool_idx /* color by a tool number */ : 0; // color by a feature type
        m_choice_view_type->SetSelection(type);
        if ((0 <= type) && (type < (int)GCodePreviewData::Extrusion::Num_View_Types))
            m_gcode_preview_data->extrusion.view_type = (GCodePreviewData::Extrusion::EViewType)type;
        // If the->SetSelection changed the following line, revert it to "decide yourself".
        m_preferred_color_mode = "tool_or_feature";
    }

    // Collect colors per extruder.
    std::vector<std::string> colors;
    if (!m_gcode_preview_data->empty() || (m_gcode_preview_data->extrusion.view_type == GCodePreviewData::Extrusion::Tool))
    {
        const ConfigOptionStrings* extruders_opt = dynamic_cast<const ConfigOptionStrings*>(m_config->option("extruder_colour"));
        const ConfigOptionStrings* filamemts_opt = dynamic_cast<const ConfigOptionStrings*>(m_config->option("filament_colour"));
        unsigned int colors_count = std::max((unsigned int)extruders_opt->values.size(), (unsigned int)filamemts_opt->values.size());

        unsigned char rgb[3];
        for (unsigned int i = 0; i < colors_count; ++i)
        {
            std::string color = m_config->opt_string("extruder_colour", i);
            if (!PresetBundle::parse_color(color, rgb))
            {
                color = m_config->opt_string("filament_colour", i);
                if (!PresetBundle::parse_color(color, rgb))
                    color = "#FFFFFF";
            }

            colors.push_back(color);
        }
    }

    if (IsShown())
    {
        // used to set the sliders to the extremes of the current zs range
        m_force_sliders_full_range = false;

        if (m_gcode_preview_data->empty())
        {
            // load skirt and brim
            m_canvas->load_preview(colors);
            show_hide_ui_elements("simple");
        }
        else
        {
            m_force_sliders_full_range = (m_canvas->get_volumes_count() == 0);
            m_canvas->load_gcode_preview(*m_gcode_preview_data, colors);
            show_hide_ui_elements("full");

            // recalculates zs and update sliders accordingly
            n_layers = (unsigned int)m_canvas->get_current_print_zs( true).size();
            if (n_layers == 0)
            {
                // all layers filtered out
                reset_sliders();
                m_canvas_widget->Refresh();
            }
        }

        if (n_layers > 0)
            update_sliders();

        m_loaded = true;
    }
}

void Preview::reload_print(bool force)
{
    m_canvas->reset_volumes();
    m_loaded = false;

    if (!IsShown() && !force)
        return;

    load_print();
}

void Preview::refresh_print()
{
    m_loaded = false;

    if (!IsShown())
        return;

    load_print();
}

void Preview::bind_event_handlers()
{
    this->Bind(wxEVT_SIZE, &Preview::on_size, this);
    m_choice_view_type->Bind(wxEVT_CHOICE, &Preview::on_choice_view_type, this);
    m_combochecklist_features->Bind(wxEVT_CHECKLISTBOX, &Preview::on_combochecklist_features, this);
    m_checkbox_travel->Bind(wxEVT_CHECKBOX, &Preview::on_checkbox_travel, this);
    m_checkbox_retractions->Bind(wxEVT_CHECKBOX, &Preview::on_checkbox_retractions, this);
    m_checkbox_unretractions->Bind(wxEVT_CHECKBOX, &Preview::on_checkbox_unretractions, this);
    m_checkbox_shells->Bind(wxEVT_CHECKBOX, &Preview::on_checkbox_shells, this);
}

void Preview::unbind_event_handlers()
{
    this->Unbind(wxEVT_SIZE, &Preview::on_size, this);
    m_choice_view_type->Unbind(wxEVT_CHOICE, &Preview::on_choice_view_type, this);
    m_combochecklist_features->Unbind(wxEVT_CHECKLISTBOX, &Preview::on_combochecklist_features, this);
    m_checkbox_travel->Unbind(wxEVT_CHECKBOX, &Preview::on_checkbox_travel, this);
    m_checkbox_retractions->Unbind(wxEVT_CHECKBOX, &Preview::on_checkbox_retractions, this);
    m_checkbox_unretractions->Unbind(wxEVT_CHECKBOX, &Preview::on_checkbox_unretractions, this);
    m_checkbox_shells->Unbind(wxEVT_CHECKBOX, &Preview::on_checkbox_shells, this);
}

void Preview::show_hide_ui_elements(const std::string& what)
{
    bool enable = (what == "full");
    m_label_show_features->Enable(enable);
    m_combochecklist_features->Enable(enable);
    m_checkbox_travel->Enable(enable); 
    m_checkbox_retractions->Enable(enable);
    m_checkbox_unretractions->Enable(enable);
    m_checkbox_shells->Enable(enable);

    enable = (what != "none");
    m_label_view_type->Enable(enable);
    m_choice_view_type->Enable(enable);
}

void Preview::reset_sliders()
{
    m_enabled = false;
    reset_double_slider();
    m_double_slider_sizer->Hide((size_t)0);
}

void Preview::update_sliders()
{
    m_enabled = true;
    update_double_slider(m_force_sliders_full_range);
    m_double_slider_sizer->Show((size_t)0);
    Layout();
}

void Preview::on_size(wxSizeEvent& evt)
{
    evt.Skip();
    Refresh();
}

void Preview::on_choice_view_type(wxCommandEvent& evt)
{
    m_preferred_color_mode = (m_choice_view_type->GetStringSelection() == L("Tool")) ? "tool" : "feature";
    int selection = m_choice_view_type->GetCurrentSelection();
    if ((0 <= selection) && (selection < (int)GCodePreviewData::Extrusion::Num_View_Types))
        m_gcode_preview_data->extrusion.view_type = (GCodePreviewData::Extrusion::EViewType)selection;

    reload_print();
}

void Preview::on_combochecklist_features(wxCommandEvent& evt)
{
    int flags = Slic3r::GUI::combochecklist_get_flags(m_combochecklist_features);
    m_gcode_preview_data->extrusion.role_flags = (unsigned int)flags;
    refresh_print();
}

void Preview::on_checkbox_travel(wxCommandEvent& evt)
{
    m_gcode_preview_data->travel.is_visible = m_checkbox_travel->IsChecked();
    refresh_print();
}

void Preview::on_checkbox_retractions(wxCommandEvent& evt)
{
    m_gcode_preview_data->retraction.is_visible = m_checkbox_retractions->IsChecked();
    refresh_print();
}

void Preview::on_checkbox_unretractions(wxCommandEvent& evt)
{
    m_gcode_preview_data->unretraction.is_visible = m_checkbox_unretractions->IsChecked();
    refresh_print();
}

void Preview::on_checkbox_shells(wxCommandEvent& evt)
{
    m_gcode_preview_data->shell.is_visible = m_checkbox_shells->IsChecked();
    refresh_print();
}

void Preview::create_double_slider()
{
    m_slider = new PrusaDoubleSlider(this, wxID_ANY, 0, 0, 0, 100);
    m_double_slider_sizer->Add(m_slider, 0, wxEXPAND, 0);

    // sizer, m_canvas_widget
    m_canvas_widget->Bind(wxEVT_KEY_DOWN, &Preview::update_double_slider_from_canvas, this);

    m_slider->Bind(wxEVT_SCROLL_CHANGED, [this](wxEvent& event) {
        m_canvas->set_toolpaths_range(m_slider->GetLowerValueD() - 1e-6, m_slider->GetHigherValueD() + 1e-6);
        if (IsShown())
            m_canvas_widget->Refresh();
    });

    Bind(wxCUSTOMEVT_TICKSCHANGED, [this](wxEvent&) {
            auto& config = wxGetApp().preset_bundle->project_config;
            ((config.option<ConfigOptionFloats>("colorprint_heights"))->values) = (m_slider->GetTicksValues());
            m_schedule_background_process();
        });
}

void Preview::update_double_slider(bool force_sliders_full_range)
{
    std::vector<std::pair<int, double>> values;
    std::vector<double> layers_z = m_canvas->get_current_print_zs(true);
    fill_slider_values(values, layers_z);

    const double z_low = m_slider->GetLowerValueD();
    const double z_high = m_slider->GetHigherValueD();
    m_slider->SetMaxValue(layers_z.size() - 1);
    m_slider->SetSliderValues(values);

    const auto& config = wxGetApp().preset_bundle->project_config;
    const std::vector<double> &ticks_from_config = (config.option<ConfigOptionFloats>("colorprint_heights"))->values;

    m_slider->SetTicksValues(ticks_from_config);

    set_double_slider_thumbs(force_sliders_full_range, layers_z, z_low, z_high);
}

void Preview::fill_slider_values(std::vector<std::pair<int, double>> &values,
                                 const std::vector<double> &layers_z)
{
    std::vector<double> layers_all_z = m_canvas->get_current_print_zs(false);
    if (layers_all_z.size() == layers_z.size())
        for (int i = 0; i < layers_z.size(); i++)
            values.push_back(std::pair<int, double>(i + 1, layers_z[i]));
    else if (layers_all_z.size() > layers_z.size()) {
        int cur_id = 0;
        for (int i = 0; i < layers_z.size(); i++)
            for (int j = cur_id; j < layers_all_z.size(); j++)
                if (layers_z[i] - 1e-6 < layers_all_z[j] && layers_all_z[j] < layers_z[i] + 1e-6) {
                    values.push_back(std::pair<int, double>(j + 1, layers_z[i]));
                    cur_id = j;
                    break;
                }
    }

    // All ticks that would end up outside the slider range should be erased.
    // TODO: this should be placed into more appropriate part of code,
    // this function is e.g. not called when the last object is deleted
    auto& config = wxGetApp().preset_bundle->project_config;
    std::vector<double> &ticks_from_config = (config.option<ConfigOptionFloats>("colorprint_heights"))->values;
    unsigned int old_size = ticks_from_config.size();
    ticks_from_config.erase(std::remove_if(ticks_from_config.begin(), ticks_from_config.end(),
                                           [values](double val) { return values.back().second < val; }),
                            ticks_from_config.end());
    if (ticks_from_config.size() != old_size)
        m_schedule_background_process();
}

void Preview::set_double_slider_thumbs(const bool force_sliders_full_range,
                                       const std::vector<double> &layers_z,
                                       const double z_low, 
                                       const double z_high)
{
    // Force slider full range only when slider is created.
    // Support selected diapason on the all next steps
    if (/*force_sliders_full_range*/z_high == 0.0) {
        m_slider->SetLowerValue(0);
        m_slider->SetHigherValue(layers_z.size() - 1);
        return;
    }

    for (int i = layers_z.size() - 1; i >= 0; i--)
        if (z_low >= layers_z[i]) {
            m_slider->SetLowerValue(i);
            break;
        }
    for (int i = layers_z.size() - 1; i >= 0; i--)
        if (z_high >= layers_z[i]) {
            m_slider->SetHigherValue(i);
            break;
        }
}

void Preview::reset_double_slider()
{
    m_slider->SetHigherValue(0);
    m_slider->SetLowerValue(0);
}

void Preview::update_double_slider_from_canvas(wxKeyEvent& event)
{
    if (event.HasModifiers()) {
        event.Skip();
        return;
    }

    const auto key = event.GetKeyCode();

    if (key == 'U' || key == 'D') {
        const int new_pos = key == 'U' ? m_slider->GetHigherValue() + 1 : m_slider->GetHigherValue() - 1;
        m_slider->SetHigherValue(new_pos);
        if (event.ShiftDown()) m_slider->SetLowerValue(m_slider->GetHigherValue());
    }
    else if (key == 'S')
        m_slider->ChangeOneLayerLock();
    else
        event.Skip();
}

} // namespace GUI
} // namespace Slic3r
