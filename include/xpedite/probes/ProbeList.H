///////////////////////////////////////////////////////////////////////////////
//
// ProbeList - A collection of iterable probes
//
// Provides methods to
//   1. Lazy initialize thread sample buffers
//   2. Logic to locate, enable and disable probes
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <iterator>
#include <vector>
#include <xpedite/probes/Probe.H>

namespace xpedite { namespace probes {

  class ProbeList
  {
    Probe* _head;
    unsigned _size;

    static ProbeList* _instance;

    public:

    ProbeList()
      : _head {}, _size {} {
    }

    unsigned size() const noexcept {
      return _size;
    }

    bool add(Probe* probe_) {
      probe_->_id = _size++;
      probe_->_prev = nullptr;
      probe_->_next = _head;
      if(_head) {
        _head->_prev = probe_;
      }
      _head = probe_;
      return true;
    }

    bool remove(Probe* probe_) {
      if(isValid(probe_)) {
        if(probe_->_next) {
          probe_->_next->_prev = probe_->_prev;
        }

        if(probe_->_prev) {
          probe_->_prev->_next = probe_->_next;
        }

        if(_head == probe_) {
          _head = probe_->_next ? probe_->_next : probe_->_prev;
        }
        --_size;
        return true;
      }
      return {};
    }

    bool isValid(Probe* probe_) const noexcept {
      if(probe_->_next && probe_->_next->_prev != probe_) {
        XpediteLogCritical << "detected probe list corruption - probe " << probe_ << " not chained to next probe " 
          << probe_->_next << XpediteLogEnd;
        return {};
      }

      if(probe_->_prev && probe_->_prev->_next != probe_) {
        XpediteLogCritical << "detected probe list corruption - probe " << probe_ << " not chained to prev probe "
          << probe_->_prev << XpediteLogEnd;
        return {};
      }
      return true;
    }

    std::vector<Probe*> findByName(const char* name_) const noexcept {
      std::vector<Probe*> probes {};
      for(auto& probe : *this) {
        if(probe.matchName(name_)) {
          probes.emplace_back(&probe);
        }
      }
      return probes;
    }

    std::vector<Probe*> findByLocation(const char* file_, uint32_t line_) const noexcept {
      std::vector<Probe*> probes {};
      for(auto& probe : *this) {
        if(probe.matchLocation(file_, line_)) {
          probes.emplace_back(&probe);
        }
      }
      return probes;
    }

    Probe* findByReturnSite(const void* returnSite_) const noexcept {
      for(auto& probe : *this) {
        if(probe.recorderReturnSite() == returnSite_) {
          return &probe;
        }
      }
      return {};
    }

    class Iterator : public std::iterator<std::forward_iterator_tag, probes::Probe>
    {
      Probe* _probe;

      public:

      explicit Iterator(Probe* probe_)
        : _probe {probe_} {
      }

      Iterator& operator++() {
        if(_probe) {
          _probe = _probe->_next;
        }
        return *this;
      }

      Iterator operator++(int) {
        Iterator i {*this};
        operator++();
        return i;
      }

      bool operator==(Iterator other_) const { return _probe == other_._probe; }
      bool operator!=(Iterator other_) const { return !(*this == other_);      }

      reference operator*() const {
        return *_probe;
      }
    };

    Iterator begin() const { return Iterator {_head};   }
    Iterator end()   const { return Iterator {nullptr}; }

    static ProbeList& get() noexcept {
      if(!_instance) {
        _instance = new ProbeList {};
      }
      return *_instance;
    }
  };

  inline const ProbeList& probeList() {
    return ProbeList::get();
  }
}}
