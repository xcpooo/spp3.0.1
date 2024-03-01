#ifndef _SINGLE_TON_H
#define _SINGLE_TON_H


namespace spp
{

    namespace singleton
    {

        template<class T>  class CreateNew
        {
        public:
            static T* Instance(T* Proto) {
                return new T;
            }
            static void Destroy(T* obj) {
                delete obj;
            }
        };


        template<class T>  class CreateByProto
        {
        public:
            static T* Instance(T* Proto) {
                return Proto;
            }

            static void Destroy(T* obj) {
                return;
            }
        };


        template < class T, template<class> class CreatePolicy = CreateNew  > class  SingleTon
        {
        public:

            static T* Instance() {
                if (Instance_ == NULL) {
                    Instance_ = CreatePolicy<T>::Instance(ProtoInstance_);
                }

                return  Instance_;
            }

            static void Destroy() {
                CreatePolicy<T>::Destroy(Instance_);
            }

            static void SetProto(T* proto) {
                Instance_ = NULL;
                ProtoInstance_ = proto;
                //	Instance_=NULL;
            }

            static T*  Instance_;
            static T*  ProtoInstance_;
        private:
            SingleTon(void);
            SingleTon(const SingleTon&);
            SingleTon& operator= (const SingleTon&);
        };


        template <class T, template <class> class CreationPolicy>
        T* SingleTon <T, CreationPolicy >::Instance_ = NULL;

        template <class T, template <class> class CreationPolicy>
        T* SingleTon <T, CreationPolicy >::ProtoInstance_ = NULL;
    }
}


#endif
