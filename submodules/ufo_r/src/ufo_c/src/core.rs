use std::{
    collections::HashMap,
    sync::{Arc, RwLock},
};

use libc::c_void;
use ufo_core::{UfoCoreConfig, UfoId, UfoObjectParams, UfoPopulateError, UfoWritebackListenerFn};
use ufo_core::sizes::*;

use crate::UfoPopulateData;

use super::*;

pub type UfoEventCallbackData = *mut libc::c_void;
pub type UfoEventCallback = extern "C" fn(UfoEventCallbackData, &UfoEventandTimestamp);

/// Rust uses closures, but C uses callback functions and data pointers
/// store these parameters ourselves
pub(crate) struct CParams {
    pub(crate) populate_data: UfoPopulateData,
    pub(crate) populate_fn: UfoPopulateCallout,

    pub(crate) writeback_listener_data: UfoWritebackListenerData,
    pub(crate) writeback_listener: UfoWritebackListener,
}

pub(crate) struct UfoCCore {
    pub(crate) the_core: Arc<ufo_core::UfoCore>,
    pub(crate) data_map: RwLock<HashMap<UfoId, CParams>>,
}

#[repr(C)]
pub struct UfoCore {
    ptr: *mut c_void,
}
opaque_c_type!(UfoCore, Arc<UfoCCore>);

impl UfoCore {
    #[no_mangle]
    pub unsafe extern "C" fn ufo_new_core(
        writeback_temp_path: *const libc::c_char,
        low_water_mark: usize,
        high_water_mark: usize,
    ) -> Self {
        std::panic::catch_unwind(|| {
            let wb = std::ffi::CStr::from_ptr(writeback_temp_path)
                .to_str()
                .expect("invalid string")
                .to_string();

            let mut low_water_mark = low_water_mark;
            let mut high_water_mark = high_water_mark;

            if low_water_mark > high_water_mark {
                std::mem::swap(&mut low_water_mark, &mut high_water_mark);
            }
            assert!(low_water_mark < high_water_mark);

            let config = UfoCoreConfig {
                writeback_temp_path: wb,
                low_watermark: low_water_mark,
                high_watermark: high_water_mark,
            };

            let core = ufo_core::UfoCore::new(config);
            match core {
                Err(_) => Self::none(),
                Ok(core) => Self::wrap(Arc::new(UfoCCore {
                    the_core: core,
                    data_map: RwLock::new(HashMap::new()),
                })),
            }
        })
        .unwrap_or_else(|_| Self::none())
    }

    #[no_mangle]
    pub extern "C" fn ufo_core_shutdown(self) {
        std::panic::catch_unwind(|| {
            if let Some(core) = self.deref() {
                core.the_core.shutdown();
            }
        }).expect("error during shutdown");
    }

    #[no_mangle]
    pub extern "C" fn ufo_core_is_error(&self) -> bool {
        self.deref().is_none()
    }

    #[no_mangle]
    pub extern "C" fn ufo_get_by_address(&self, ptr: *mut libc::c_void) -> UfoObj {
        std::panic::catch_unwind(|| {
            self.deref()
                .and_then(|core| {
                    core.the_core
                        .get_ufo_by_address(ptr as usize)
                        .ok() // okay if this fails, we just return "none"
                        .map(UfoObj::wrap)
                })
                .unwrap_or_else(UfoObj::none)
        })
        .unwrap_or_else(|_| UfoObj::none())
    }

    #[no_mangle]
    pub extern "C" fn ufo_get_params(&self, ufo: &UfoObj, params: *mut UfoParameters) -> i32 {
        return std::panic::catch_unwind(|| {
            self.deref()
                .zip(ufo.deref())
                .and_then(|(core, ufo)| {
                    let ufo = ufo.read().expect("can't lock ufo");
                    let map = core.data_map.read().expect("can't lock map");

                    let ufo_dat = map.get(&ufo.id)?;
                    let params = unsafe { &mut *params };

                    params.header_size = ufo.config.header_size().bytes;
                    params.element_size = ufo.config.stride().alignment_quantum().bytes;
                    params.element_ct = ufo.config.element_ct().total().elements;
                    params.min_load_ct = ufo.config.elements_loaded_at_once().alignment_quantum().elements;
                    params.read_only = ufo.config.read_only();
                    params.populate_data = ufo_dat.populate_data;
                    params.populate_fn = ufo_dat.populate_fn;
                    params.writeback_listener_data = ufo_dat.writeback_listener_data;
                    params.writeback_listener = ufo_dat.writeback_listener;

                    Some(0)
                })
                .unwrap_or(-1)
        })
        .unwrap_or(-2);
    }

    #[no_mangle]
    pub extern "C" fn ufo_address_is_ufo_object(&self, ptr: *mut libc::c_void) -> bool {
        std::panic::catch_unwind(|| {
            self.deref()
                .and_then(|core| {
                    core.the_core.get_ufo_by_address(ptr as usize).ok()?; // don't care about the error, just doing an is-UFO
                    Some(true)
                })
                .unwrap_or(false)
        })
        .unwrap_or(false)
    }

    #[no_mangle]
    pub extern "C" fn ufo_new_object(&self, prototype: &UfoParameters) -> UfoObj {
        std::panic::catch_unwind(|| {
            let populate_data = prototype.populate_data as usize;
            let populate_fn = prototype.populate_fn;
            let populate = move |start, end, to_populate| {
                let ret = populate_fn(populate_data as *mut c_void, start, end, to_populate);

                if ret != 0 {
                    Err(UfoPopulateError)
                } else {
                    Ok(())
                }
            };

            let writeback_listener: Option<Box<UfoWritebackListenerFn>>;
            if let Some(c_listener) = prototype.writeback_listener {
                let writeback_listener_data = prototype.writeback_listener_data as usize;
                let raw_listener =
                    move |event| c_listener(writeback_listener_data as *mut c_void, event);
                writeback_listener = Some(Box::new(raw_listener));
            } else {
                writeback_listener = None;
            }

            let params = UfoObjectParams {
                header_size: prototype.header_size,
                stride: prototype.element_size,
                element_ct: prototype.element_ct,
                min_load_ct: Some(prototype.min_load_ct).filter(|x| *x > 0),
                read_only: prototype.read_only,
                populate: Box::new(populate),
                writeback_listener,
            };

            self.deref()
                .and_then(move |core| {
                    let ufo = core.the_core.allocate_ufo(params.new_config());
                    match ufo {
                        Ok(ufo) => {
                            let mut data_map =
                                core.data_map.write().expect("unable to lock data map");
                            let id = ufo.read().expect("can't get read lock").id;
                            data_map.insert(
                                id,
                                CParams {
                                    populate_data: prototype.populate_data,
                                    populate_fn: prototype.populate_fn,

                                    writeback_listener_data: prototype.writeback_listener_data,
                                    writeback_listener: prototype.writeback_listener,
                                },
                            );

                            Some(UfoObj::wrap(ufo))
                        }
                        _ => None,
                    }
                })
                .unwrap_or_else(|| UfoObj::none())
        })
        .unwrap_or_else(|_| UfoObj::none())
    }

    #[no_mangle]
    pub extern "C" fn ufo_new_event_handler(
        &self,
        callback_data: UfoEventCallbackData,
        callback: UfoEventCallback,
    ) -> bool {
        std::panic::catch_unwind(|| {
            self.deref()
                .and_then(|core| {
                    let masked_callback_data = callback_data as usize;
                    let cb: Option<Box<ufo_core::UfoEventConsumer>> = Some(Box::new(move |e| {
                        callback(masked_callback_data as *mut c_void, e)
                    }));
                    core.the_core
                        .new_event_callback(cb)
                        .expect("error installing new event callback");
                    Some(true)
                })
                .unwrap_or(false)
        })
        .unwrap_or(false)
    }

    #[no_mangle]
    pub extern "C" fn ufo_clear_event_handler(&self) -> bool {
        std::panic::catch_unwind(|| {
            self.deref()
                .and_then(|core| {
                    core.the_core
                        .new_event_callback(None)
                        .expect("error installing new event callback");
                    Some(true)
                })
                .unwrap_or(false)
        })
        .unwrap_or(false)
    }
}
