
// c++
#include <exception>
#include <string>

// node.js
#include <node.h>
#include <node_object_wrap.h>

// osmium
#include <osmium/visitor.hpp>

// node-osmium
#include "reader_wrap.hpp"
#include "file_wrap.hpp"
#include "handler.hpp"
#include "osm_object_wrap.hpp"

namespace node_osmium {

    Persistent<FunctionTemplate> ReaderWrap::constructor;

    void ReaderWrap::Initialize(Handle<Object> target) {
        HandleScope scope;
        constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(ReaderWrap::New));
        constructor->InstanceTemplate()->SetInternalFieldCount(1);
        constructor->SetClassName(String::NewSymbol("Reader"));
        NODE_SET_PROTOTYPE_METHOD(constructor, "header", header);
        NODE_SET_PROTOTYPE_METHOD(constructor, "apply", apply);
        NODE_SET_PROTOTYPE_METHOD(constructor, "close", close);
        target->Set(String::NewSymbol("Reader"), constructor->GetFunction());
    }

    Handle<Value> ReaderWrap::New(const Arguments& args) {
        HandleScope scope;
        if (!args.IsConstructCall()) {
            return ThrowException(Exception::Error(String::New("Cannot call constructor as function, you need to use 'new' keyword")));
        }
        if (args.Length() < 1 || args.Length() > 2) {
            return ThrowException(Exception::TypeError(String::New("please provide a File object or string for the first argument and optional options Object when creating a Reader")));
        }
        try {
            osmium::osm_entity::flags read_which_entities = osmium::osm_entity::flags::all;
            if (args.Length() == 2) {
                if (!args[1]->IsObject()) {
                    return ThrowException(Exception::TypeError(String::New("Second argument to Reader constructor must be object")));
                }
                read_which_entities = osmium::osm_entity::flags::nothing;
                Local<Object> options = args[1]->ToObject();

                Local<Value> want_nodes = options->Get(String::New("node"));
                if (want_nodes->IsBoolean() && want_nodes->BooleanValue()) {
                    read_which_entities |= osmium::osm_entity::flags::node;
                }

                Local<Value> want_ways = options->Get(String::New("way"));
                if (want_ways->IsBoolean() && want_ways->BooleanValue()) {
                    read_which_entities |= osmium::osm_entity::flags::way;
                }

                Local<Value> want_relations = options->Get(String::New("relation"));
                if (want_relations->IsBoolean() && want_relations->BooleanValue()) {
                    read_which_entities |= osmium::osm_entity::flags::relation;
                }

            }
            if (args[0]->IsString()) {
                osmium::io::File file(*String::Utf8Value(args[0]));
                ReaderWrap* q = new ReaderWrap(file, read_which_entities);
                q->Wrap(args.This());
                return args.This();
            } else if (args[0]->IsObject() && FileWrap::constructor->HasInstance(args[0]->ToObject())) {
                Local<Object> file_obj = args[0]->ToObject();
                FileWrap* file_wrap = node::ObjectWrap::Unwrap<FileWrap>(file_obj);
                ReaderWrap* q = new ReaderWrap(*(file_wrap->get()), read_which_entities);
                q->Wrap(args.This());
                return args.This();
            } else {
                return ThrowException(Exception::TypeError(String::New("please provide a File object or string for the first argument when creating a Reader")));
            }
        } catch (const std::exception& ex) {
            return ThrowException(Exception::TypeError(String::New(ex.what())));
        }
        return Undefined();
    }

    Handle<Value> ReaderWrap::header(const Arguments& args) {
        HandleScope scope;
        Local<Object> obj = Object::New();
        ReaderWrap* reader = node::ObjectWrap::Unwrap<ReaderWrap>(args.This());
        const osmium::io::Header& header = reader->m_this->header();
        obj->Set(String::New("generator"), String::New(header.get("generator").c_str()));
        const osmium::Box& bounds = header.box();
        Local<Array> arr = Array::New(4);
        arr->Set(0, Number::New(bounds.bottom_left().lon()));
        arr->Set(1, Number::New(bounds.bottom_left().lat()));
        arr->Set(2, Number::New(bounds.top_right().lon()));
        arr->Set(3, Number::New(bounds.top_right().lat()));
        obj->Set(String::New("bounds"), arr);
        return scope.Close(obj);
    }

    Handle<Value> ReaderWrap::apply(const Arguments& args) {
        HandleScope scope;

        if (args.Length() != 1 && args.Length() != 2) {
            return ThrowException(Exception::TypeError(String::New("please provide a single handler object")));
        }
        if (!args[0]->IsObject()) {
            return ThrowException(Exception::TypeError(String::New("please provide a single handler object")));
        }
        Local<Object> obj = args[0]->ToObject();
        if (obj->IsNull() || obj->IsUndefined() || !JSHandler::constructor->HasInstance(obj)) {
            return ThrowException(Exception::TypeError(String::New("please provide a valid handler object")));
        }

        bool with_location_handler = false;

        if (args.Length() == 2) {
            if (!args[1]->IsObject()) {
                return ThrowException(Exception::TypeError(String::New("second argument must be 'option' object")));
            }
            Local<Value> wlh = args[1]->ToObject()->Get(String::New("with_location_handler"));
            if (wlh->BooleanValue()) {
                with_location_handler = true;
            }
        }

        JSHandler* handler = node::ObjectWrap::Unwrap<JSHandler>(obj);
        osmium::io::Reader& reader = wrapped(args.This());

        if (with_location_handler) {
            index_pos_type index_pos;
            index_neg_type index_neg;
            location_handler_type location_handler(index_pos, index_neg);

            input_iterator it(reader);
            input_iterator end;

            for (; it != end; ++it) {
                osmium::apply_item(*it, location_handler);
                handler->dispatch_object(it);
            }

            handler->done();
        } else {
            input_iterator it(reader);
            input_iterator end;

            for (; it != end; ++it) {
                handler->dispatch_object(it);
            }

            handler->done();
        }

        return Undefined();
    }

    Handle<Value> ReaderWrap::close(const Arguments& args) {
        wrapped(args.This()).close();
        return Undefined();
    }

} // namespace node_osmium

