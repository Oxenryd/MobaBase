//#ifndef EVENT_HPP
//#define EVENT_HPP
//#pragma once
//
//#include <vector>
//#include <cstdint>
//#include <utility>
//
//
//
//
//template<typename... Args>
//class Event
//{
//    using Callback = void(*)(void* context, Args...);
//    struct Listener
//    {
//        Callback callback;
//        void* context;
//
//        void invoke(Args... args) const {
//            callback(context, args...);
//        }
//    };
//private:
//    std::vector<Listener> m_listeners;
//
//public:
//    template <typename T>
//    void subscribe(T* instance,  void (T::*method)(Args...)) {
//        Listener l;
//        l.callback = [](void* ctx, Args... args)
//            {
//                T* obj = static_cast<T*>(ctx);
//                (obj->*method)(args...);
//            };
//        l.context = instance;
//        m_listeners.push_back(l);
//    }
//
//    template <typename T>
//    void unsubscribe(T* instance) {
//        m_listeners.erase(
//            std::remove_if(
//                m_listeners.begin(), m_listeners.end(),
//                [instance](const Listener& l) { return l.context == instance; }),
//            m_listeners.end());
//    }
//
//    void unsubscribeAll() {
//        m_listeners.clear();
//    }
//
//    void notify(Args... args) const {
//        for (const auto& listener : m_listeners)
//            listener.invoke(args...);
//    }
//};
//
//
//
//template<typename... Args>
//class EventBroker
//{
//    using Listener = void(*)(Args...);
//    using EventData = std::tuple<Args...>;
//
//private:
//    std::vector<Listener> m_listeners;
//    std::vector<EventData*> m_eventQueue[2];
//    size_t m_activeQueue = 0;
//
//public:
//    void subscribe(uint32_t entityId, const Listener& listener) {
//        m_listeners.insert_or_assign(entityId, listener);
//    }
//
//    void unsubscribe(uint32_t entityId) {
//
//        m_listeners.erase(entityId);
//    }
//
//    void unsubscribeAll() {
//        m_listeners.clear();
//    }
//
//    void queue(Args... args) {
//        m_eventQueue[m_activeQueue].emplace_back(std::make_tuple(args...));
//        //m_eventQueue.emplace(std::make_tuple(args...));
//    }
//
//    void dispatch() {
//
//        size_t processQueue = m_activeQueue;
//        m_activeQueue = 1 - m_activeQueue;
//        m_eventQueue[m_activeQueue].clear();
//
//        for (const auto& event : m_eventQueue[processQueue]) {
//            std::apply([&](Args... args) {
//                for (const auto& [id, listener] : m_listeners)
//                    listener(args...);
//                       }, event);
//        }
//
//        //while (!m_eventQueue.empty()) {
//        //    const auto& event = m_eventQueue.front();
//        //    std::apply([&](Args... args) {
//        //        for (const auto& [id, listener] : m_listeners)
//        //            listener(args...);
//        //               }, event);
//        //    m_eventQueue.pop();
//        //}
//    }
//};
//
//#endif