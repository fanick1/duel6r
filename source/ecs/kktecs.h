//============================================================================
// Name        : kktecs.cpp
// Author      : fanick
// Version     :
// Copyright   : Copyright 2016, 2017, 2018 Frantisek Veverka
//============================================================================
#ifndef _KKTECS_H
#define _KKTECS_H

#include <memory>
#include <iostream>
#include <bitset>
#include <vector>
#include <stack>
//#include <cstdarg>

#define KKT_MAX_SYSTEM_COUNT 32
#define KKT_MAX_COMPONENT_COUNT 32

/** KktComponentBaseShit =============================================================**/
class KktComponentBaseShit {
    public:
        KktComponentBaseShit(){};
        KktComponentBaseShit(const KktComponentBaseShit & c){};
        static int component_type_counter;
        virtual ~KktComponentBaseShit(){}
};
//int KktComponentBaseShit::component_type_counter = 0;

/** KktComponent<C> =================================================================**/
template<class C>
class KktComponent: public KktComponentBaseShit {

    public:
        KktComponent(){}
        KktComponent(KktComponent & c){

        };
        typedef C Type;
        static C T(){
            C * c;
            return *c;
        }
        static int ComponentId(){
            static int componentId = component_type_counter++;
            return componentId;
        }
        virtual ~KktComponent(){}
};

class KktFckr;




/** KktSystemBaseShit ===============================================================**/
class KktSystemBaseShit{
    public:
        KktSystemBaseShit(){};
        static int system_type_counter;
        virtual void update(float timeDelta, KktFckr& fckr) = 0;
        virtual ~KktSystemBaseShit(){};
};
//int KktSystemBaseShit::system_type_counter = 0;

/** KktSystem<C> ====================================================================**/
template<class C>
class KktSystem : public KktSystemBaseShit{
    public:
        typedef C Type;
        static int SystemId(){
            static int SystemId = system_type_counter++;
            return SystemId;
        }
        virtual int systemId(){
            return SystemId();
        }
        virtual ~KktSystem(){}
};
/** KktFckr =========================================================================**/
class KktFckr{
public:
    /** KktEntity =======================================================================**/
    class KktEntity{
    public:
        typedef std::bitset<KKT_MAX_COMPONENT_COUNT> mask_t;

    public:
        KktEntity(unsigned int id, KktFckr & kktFckr):id(id), kktFckr(kktFckr){};
        void destroy(){
            kktFckr.destroyEntity(id);
        }
        mask_t components;
        unsigned int id = 0;
        KktFckr & kktFckr;
    };
private:
    struct EntityBookKeepingRecord {
        size_t prev;
        size_t next;
        bool valid = true; // entity is live;
    };
public:
    unsigned int entityCount = 0;
    unsigned int startIndex = 0; // index to bookKeeping;
    std::shared_ptr<KktSystemBaseShit> systemPileOfShit[KKT_MAX_SYSTEM_COUNT];
    std::vector<KktEntity> pileOfEntities;
    std::vector<EntityBookKeepingRecord> bookKeeping;
    std::stack<size_t> freeSlots;
    std::vector<std::unique_ptr<KktComponentBaseShit> > pilesOfComponents[KKT_MAX_COMPONENT_COUNT];  // e.g. 32 vectors, 1 per component


    template<class ... Components>
    std::vector<int> getComponentIds() const
    {
        return std::vector<int> ({(Components::ComponentId())...});
    }
//
//    template<class ... Components>
//    std::vector<KktEntity*> getEntitiesWithComponents() {
//        return getEntitiesWithComponents(getComponentIds<Components...>());
//    }

    template<class ... Components>
    std::vector<KktEntity*> getEntitiesWithComponents() {
        return getEntitiesWithComponents({(Components::ComponentId())...});
    }

    std::vector<KktEntity*> getEntitiesWithComponents(const std::vector<int> & componentIds);
    void destroyEntity( unsigned int index);
    KktFckr::KktEntity & createEntity() ;
    // Example usage
    //                 auto p = std::make_shared<PositionComponent>();
    //                 auto c = std::make_shared<ColorComponent>();
    // also works:     fckr.get(entity, p, c);
    // also works:     auto [p,c] = fckr.get(entity, PositionComponent::T(), ColorComponent::T());
    // also works:     auto [p,c] = fckr.get<PositionComponent, ColorComponent>(entity);


    void get(KktEntity & entity){

    }

    template<class Component,  class ... Components>
    void get(KktEntity & entity, Component ** component, Components ** ... components){
        get(entity, components...);
       // Component* x = (Component*)pilesOfComponents[Component::ComponentId()][entity.id].get();
        *component = (Component*)pilesOfComponents[Component::ComponentId()][entity.id].get();

    }
//    template<class Component>
//    std::tuple<Component*> get(KktEntity & entity){
//        return std::make_tuple((Component*)(pilesOfComponents[Component::ComponentId()][entity.id].get()));
//    }
//
//    template<class Component, class ...Components>
//    std::tuple<std::shared_ptr<Component>, std::shared_ptr<Components...>> get(KktEntity & entity){
//        return std::tuple_cat(std::make_tuple(std::dynamic_pointer_cast<Component>(pilesOfComponents[Component::ComponentId()][entity.id])), get<Components...>(entity));
//    }
//    template<class Component>
//    std::tuple<std::shared_ptr<Component>> get(KktEntity & entity, const Component & C){
//        return std::make_tuple(std::dynamic_pointer_cast<Component>(pilesOfComponents[Component::ComponentId()][entity.id]));
//    }
//
//    template<class Component, class ...Components>
//    std::tuple<Component*, Components*...> get(KktEntity & entity, const Component ** C, const Components** ... CC){
//        return std::tuple_cat(std::make_tuple((Component*)(pilesOfComponents[Component::ComponentId()][entity.id]->get())), get<Components...>(entity));
//    }
//    template< class ... Component>
//    void set(KktEntity & entity, std::shared_ptr<Component> & ... components){
//        auto x = {(entity.components[components->ComponentId()] = true)...};
//        auto y = {(pilesOfComponents[components->ComponentId()][entity.id] = components)...};
//        // TODO
//    }
    template< class ... Component>
    void set(KktEntity & entity, Component & ... components){
        auto x = {(entity.components[Component::ComponentId()] = true)...};
        auto y = {setComp(entity, components)...};
//        auto & y = {(pilesOfComponents[Component::ComponentId()][entity.id].reset(new Component(components)))...};
        // TODO
    }
    template< class Component>
    int setComp(KktEntity & entity, Component & component){
        (pilesOfComponents[Component::ComponentId()][entity.id].reset(new Component(component)));
        return 0;
    }
    // removes components
    template< class ... Component>
    void remove(std::shared_ptr<KktEntity> & entity, std::shared_ptr<Component> & ... components){
        auto x = {(entity->components[components->ComponentId()] = false)...};
        auto y = {(pilesOfComponents[components->ComponentId()][entity->id] = std::make_shared<KktComponentBaseShit>())...};
        // TODO
    }
    template<class ... Components>
    KktFckr::KktEntity & createEntity(Components & ... components){
        auto & e = createEntity();
        set<Components...>(e, (components)...);

        return e;
    }
//    template<class ... Components>
//    KktFckr::KktEntity & createEntity(std::shared_ptr<Components> & ... components){
//        auto & e = createEntity();
//        set<Components...>(e, components...);
//
//        return e;
//    }

    /** registerSystem **/
    template< class T, typename ... Args>
    std::shared_ptr<T> registerSystem(Args && ...args){
        auto shit = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
        registerSystem(shit);
        return shit;
    }

    /** registerSystem **/
    template<class T>
    std::shared_ptr<T> registerSystem(){
        auto shit = std::shared_ptr<T>(new T());
        registerSystem(shit);
        return shit;
    }

    /** getSystem **/
    template<class T>
    std::shared_ptr<T> getSystem(){
        return systemPileOfShit[T::SystemId()];
    }

    /** updateAll **/
    void updateAll(float timeDelta){
        for(size_t i = 0; i < KKT_MAX_SYSTEM_COUNT; ++i){
            auto system = systemPileOfShit[i].get();
            if(system != nullptr){
                system->update(timeDelta, *this);
            }
        }
    }

    /** registerDynamicSystem **/
    template<class T>
    std::shared_ptr<T> registerScriptedSystem(){
        std::shared_ptr<T> system(new T());
        systemPileOfShit[system.get()->systemId()] = system;
        return system;
    }

    /** getScriptedSystem **/
    template<class T>
    std::shared_ptr<T> getScriptedSystem(int systemId){
        return systemPileOfShit[systemId];
    }
private:
    template<class T>
    void registerSystem(std::shared_ptr<T> system){
        systemPileOfShit[T::SystemId()] = system;
    }
};


#endif
