//============================================================================
// Name        : kktecs.cpp
// Author      : fanick
// Version     :
// Copyright   : Copyright 2016, 2017, 2018 Frantisek Veverka
//============================================================================
#include <memory>
#include <vector>
#include <iostream>
#include "kktecs.h"
int KktComponentBaseShit::component_type_counter = 0;
int KktSystemBaseShit::system_type_counter = 0;


std::vector<std::shared_ptr<KktFckr::KktEntity> > KktFckr::getEntitiesWithComponents(const std::vector<int> & componentIds) const{
     KktEntity::mask_t mask;
     for(int c : componentIds){
         mask[c] = true;
     }

     std::vector<std::shared_ptr<KktEntity> > result;
     result.reserve(128);
     size_t index = startIndex;
     bool debug = false;
     if(debug == false && bookKeeping.size() == 3){
         debug = true;
     }
     while(index < bookKeeping.size() && bookKeeping[index].valid){
         const auto & record = bookKeeping[index];
         const auto & entity = pileOfEntities[index];
         if((entity->components & mask) == mask){
             result.push_back(entity);
         }
         if(index == record.next){
             break;
         }
         index = record.next;

     }
     if(index > startIndex) std::cout << " \n";
     return result;
 }
 void KktFckr::destroyEntity( unsigned int index){
     if(index >= pileOfEntities.size()){
         //throw
         return;
     }
     freeSlots.push(index);
     auto & record = bookKeeping[index];

     auto & [prev, next, valid] (record);
     valid = false && valid; // to avoid 'unused variable warning'
     if( prev != next /*special case - destroying first and only element*/){
         if(next != index && next < pileOfEntities.size()){
             auto & nextRecord = bookKeeping[next];
             nextRecord.prev = prev;
             if(index == startIndex && nextRecord.valid) startIndex = next;

         }
         if(prev != index && prev < pileOfEntities.size()){
             auto & prevRecord = bookKeeping[prev];
             prevRecord.next = next;
             if(index == startIndex && prevRecord.valid) startIndex = prev;
         }
     }
 }
 std::shared_ptr<KktFckr::KktEntity> KktFckr::createEntity() {
     std::shared_ptr<KktEntity> entity;
     if(freeSlots.empty()){
         entity = std::shared_ptr<KktEntity>(new KktEntity(entityCount++, *this));
         pileOfEntities.push_back(entity);
         if(bookKeeping.size() == 0){
             startIndex = 0;
             bookKeeping.push_back({0,0, true});
         } else {
             auto index = entity->id;
             auto prev = index - 1;
             bookKeeping[prev].next = index;
             bookKeeping.push_back({prev, index, true});
         }
         for(auto & componentPile : pilesOfComponents){
             componentPile.push_back(std::shared_ptr<KktComponentBaseShit>(new KktComponentBaseShit()));
         }
     } else {
         auto index = freeSlots.top();
         freeSlots.pop();
         if (index < startIndex || !bookKeeping[startIndex].valid){
             startIndex = index;
         }
         auto & record = bookKeeping[index];
         auto & [prev, next, valid] (record);
         if( prev != next /*special case - first and only element*/){
             if(next != index && next < pileOfEntities.size()){
                 auto & nextRecord = bookKeeping[next];
                 nextRecord.prev = index;
             }
             if(prev != index && prev < pileOfEntities.size()){
                 auto & prevRecord = bookKeeping[prev];
                 prevRecord.next = index;
             }
         }
         valid = true || valid; // to avoid 'unused variable warning'
         entity = pileOfEntities[index];
         for(auto & componentPile : pilesOfComponents){
             componentPile[index] = std::make_shared<KktComponentBaseShit>();
             entity->components.reset();
         }
     }

     return entity;
 }