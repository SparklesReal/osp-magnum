#include "SysAreaAssociate.h"

#include "ActiveScene.h"


using namespace osp::universe;
using namespace osp::active;
using namespace osp;

SysAreaAssociate::SysAreaAssociate(ActiveScene &rScene, Universe &uni) :
       m_scene(rScene),
       m_universe(uni),
       m_updateScan(rScene.get_update_order(), "areascan", "", "",
                    std::bind(&SysAreaAssociate::update_scan, this))
{

}

void SysAreaAssociate::update_scan()
{
    // scan for nearby satellites, maybe move this somewhere else some day

    if (!m_universe.get_reg().valid(m_areaSat))
    {
        return;
    }

    //Universe& uni = m_universe;
    //std::vector<Satellite>& satellites = uni.
    auto view = m_universe.get_reg().view<UCompActivatable>();

    for (Satellite sat : view)
    {
//        if (SatelliteObject* obj = sat.get_object())
//        {
//            //std::cout << "obj: " << obj->get_id().m_name
//            //          << ", " << (void*)(&(obj->get_id())) << "\n";
//        }
//        else
//        {
//            // Satellite has no object, it's empty
//            continue;
//        }

//        // if satellite is not loadable or is already activated
//        if (!sat.is_activatable() || sat.get_loader())
//        {
//            //std::cout << "already loaded\n";
//            continue;
//        }


        // ignore self (unreachable as active areas are unloadable)
        //if (&sat == m_sat)
        //{
        //    continue;
        //}

        // Calculate distance to satellite
        // For these long vectors, 1024 units = 1 meter
        //Vector3s relativePos = m_universe.sat_calc_position(m_areaSat, sat);
        //Vector3 relativePosMeters = Vector3(relativePos) / 1024.0f;

        //using Magnum::Math::pow;

        // ignore if too far
        //if (relativePos.dot() > pow(sat.get_load_radius()
        //    + m_sat->get_load_radius(), 2.0f))
        //{
        //    continue;
        //}

        //std::cout << "is near! dist: " << relativePos.length() << "/"
        //<< (sat.get_load_radius() + m_sat->get_load_radius()) << "\n";

        // Satellite is near! Attempt to load it

        sat_activate(sat, view.get(sat));
    }


//    ACompTransform& cameraTransform = m_scene->reg_get<ACompTransform>(m_camera);
//    //cameraTransform.m_transform[3].xyz().x() += 0.1f; // move the camera right

//    // Temporary: Floating origin follow cameara
//    Magnum::Vector3i tra(cameraTransform.m_transform[3].xyz());

//    m_scene->floating_origin_translate(-Vector3(tra));
//    m_sat->set_position(m_sat->get_position()
//    + Vector3sp({tra.x() * 1024,
//    tra.y() * 1024,
//    tra.z() * 1024}, 10));
    //std::cout << "x: " << Vector3sp(m_sat->get_position()).x() << "\n";

}

void SysAreaAssociate::connect(universe::Satellite sat)
{
    // do more stuff here eventually
    m_areaSat = sat;
}

void SysAreaAssociate::disconnect()
{
    if (!m_activatedSats.empty())
    {
        // De-activate all satellites

        auto viewAct = m_scene.get_registry().view<ACompActivatedSat>();

        for (ActiveEnt ent : viewAct)
        {
            sat_deactivate(ent, viewAct.get(ent));
        }


    }

    m_areaSat = entt::null;
}

void SysAreaAssociate::sat_position_update(ActiveEnt ent)
{
    auto const &entAct = m_scene.reg_get<ACompActivatedSat>(ent);
    auto const &entTransform = m_scene.reg_get<ACompTransform>(ent);

    Satellite sat = entAct.m_sat;
    auto &satPosTraj = m_universe.get_reg()
            .get<universe::UCompPositionTrajectory>(sat);

    auto const &areaPosTraj = m_universe.get_reg()
            .get<universe::UCompPositionTrajectory>(m_areaSat);

    // 1024 units = 1 meter
    Vector3s posAreaRelative(entTransform.m_transform.translation() * 1024.0f);

    satPosTraj.m_position = areaPosTraj.m_position + posAreaRelative;
    satPosTraj.m_dirty = true;
}

void SysAreaAssociate::activator_add(universe::ITypeSatellite const* type,
                                         IActivator &activator)
{
    m_activators[type] = &activator;
}

int SysAreaAssociate::sat_activate(universe::Satellite sat,
                                   universe::UCompActivatable &satAct)
{

    if (m_activatedSats.find(sat) != m_activatedSats.end())
    {
        // satellite already loaded
        return 1;
    }

    auto &ureg = m_universe.get_reg();

    auto &satType = ureg.get<UCompType>(sat);

    //auto &areaSatAA = reg.get<UCompType>(m_areaSat);

    // see if there's a loading function for the satellite object
    auto funcMapIt =  m_activators.find(satType.m_type);

    if(funcMapIt == m_activators.end())
    {
        // no loading function exists for this satellite object type
        return -1;
    }

    //(ActiveScene& scene, osp::universe::SatActiveArea& area,
    //                universe::Satellite areaSat, universe::Satellite loadMe

    // Get activator and call the activate function
    IActivator *activator = funcMapIt->second;
    StatusActivated status
            = activator->activate_sat(m_scene, *this, m_areaSat, sat);

    if (status.m_status)
    {
        // Load failed
        return status.m_status;
    }

    // Load success

    // Add the activated satellite component
    auto &activated = m_scene.reg_emplace<ACompActivatedSat>(status.m_entity);

    activated.m_sat = sat;
    activated.m_activator = funcMapIt;
    activated.m_mutable = status.m_mutable;

    //sat.set_loader(m_sat->get_object());
    m_activatedSats.emplace(sat);

    return 0;
}

int SysAreaAssociate::sat_deactivate(ActiveEnt ent, ACompActivatedSat& entAct)
{
    IActivator *activator = entAct.m_activator->second;

    // this should never happen
    if (!m_activatedSats.contains(entAct.m_sat))
    {
        return 1;
    }

    activator->deactivate_sat(m_scene, *this, m_areaSat, entAct.m_sat, ent);

    m_activatedSats.erase(entAct.m_sat);

    return 0;
}

