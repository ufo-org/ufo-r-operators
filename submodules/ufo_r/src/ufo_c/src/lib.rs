#[macro_use]
mod ctype_wrapper;

mod core;
pub use crate::core::*;
mod prototype;
pub use crate::prototype::*;
mod object;
pub use crate::object::*;

pub use ufo_core::{UfoEvent, UfoEventandTimestamp, UfoUnloadDisposition};

#[no_mangle]
pub extern "C" fn ufo_begin_log() {
    stderrlog::new()
        // .module("ufo_core")
        .verbosity(4)
        .timestamp(stderrlog::Timestamp::Millisecond)
        .init()
        .unwrap();
}
