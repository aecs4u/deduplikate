use czkawka_core::common::model::{CheckingMethod, HashType};
use czkawka_core::common::tool_data::CommonData;
use czkawka_core::common::traits::Search;
use czkawka_core::tools::duplicate::{DuplicateFinder, DuplicateFinderParameters};
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;

// C-compatible enums
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum CCheckingMethod {
    Hash = 0,
    Name = 1,
    Size = 2,
    SizeName = 3,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum CHashType {
    Blake3 = 0,
    Crc32 = 1,
    Xxh3 = 2,
}

// Convert C enums to Rust enums
impl From<CCheckingMethod> for CheckingMethod {
    fn from(method: CCheckingMethod) -> Self {
        match method {
            CCheckingMethod::Hash => CheckingMethod::Hash,
            CCheckingMethod::Name => CheckingMethod::Name,
            CCheckingMethod::Size => CheckingMethod::Size,
            CCheckingMethod::SizeName => CheckingMethod::SizeName,
        }
    }
}

impl From<CHashType> for HashType {
    fn from(hash_type: CHashType) -> Self {
        match hash_type {
            CHashType::Blake3 => HashType::Blake3,
            CHashType::Crc32 => HashType::Crc32,
            CHashType::Xxh3 => HashType::Xxh3,
        }
    }
}

// C-compatible structures
#[repr(C)]
pub struct CDuplicateEntry {
    pub path: *const c_char,
    pub size: u64,
    pub modified_date: u64,
    pub hash: *const c_char,
}

#[repr(C)]
pub struct CDuplicateGroup {
    pub entries: *const CDuplicateEntry,
    pub count: usize,
}

#[repr(C)]
pub struct CDuplicateResults {
    pub groups: *const CDuplicateGroup,
    pub group_count: usize,
    pub total_files: usize,
    pub wasted_space: u64,
}

// Progress callback
pub type ProgressCallback = extern "C" fn(current: u64, total: u64, user_data: *mut std::ffi::c_void);

// Opaque pointer to DuplicateFinder
pub struct CzkawkaDuplicateFinder {
    finder: DuplicateFinder,
    stop_flag: Arc<AtomicBool>,
    included_paths: Vec<PathBuf>,
    excluded_paths: Vec<PathBuf>,
}

// Initialize a new duplicate finder
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_new(
    check_method: CCheckingMethod,
    hash_type: CHashType,
    ignore_hard_links: bool,
    use_cache: bool,
) -> *mut CzkawkaDuplicateFinder {
    let params = DuplicateFinderParameters::new(
        check_method.into(),
        hash_type.into(),
        ignore_hard_links,
        use_cache,
        1024 * 1024,      // minimal_cache_file_size: 1 MB
        1024 * 1024,      // minimal_prehash_cache_file_size: 1 MB
        true,             // case_sensitive_name_comparison
    );

    let finder = DuplicateFinder::new(params);
    let stop_flag = Arc::new(AtomicBool::new(false));

    Box::into_raw(Box::new(CzkawkaDuplicateFinder {
        finder,
        stop_flag,
        included_paths: Vec::new(),
        excluded_paths: Vec::new(),
    }))
}

// Free the duplicate finder
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_free(finder: *mut CzkawkaDuplicateFinder) {
    if !finder.is_null() {
        unsafe {
            let _ = Box::from_raw(finder);
        }
    }
}

// Add directory to scan
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_add_directory(
    finder: *mut CzkawkaDuplicateFinder,
    path: *const c_char,
) -> bool {
    if finder.is_null() || path.is_null() {
        return false;
    }

    unsafe {
        let finder = &mut *finder;
        let path_str = match CStr::from_ptr(path).to_str() {
            Ok(s) => s,
            Err(_) => return false,
        };

        finder.included_paths.push(PathBuf::from(path_str));
        true
    }
}

// Add excluded directory
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_add_excluded_directory(
    finder: *mut CzkawkaDuplicateFinder,
    path: *const c_char,
) -> bool {
    if finder.is_null() || path.is_null() {
        return false;
    }

    unsafe {
        let finder = &mut *finder;
        let path_str = match CStr::from_ptr(path).to_str() {
            Ok(s) => s,
            Err(_) => return false,
        };

        finder.excluded_paths.push(PathBuf::from(path_str));
        true
    }
}

// Set recursive search
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_set_recursive(
    finder: *mut CzkawkaDuplicateFinder,
    recursive: bool,
) {
    if !finder.is_null() {
        unsafe {
            (*finder).finder.set_recursive_search(recursive);
        }
    }
}

// Set minimum file size
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_set_min_size(
    finder: *mut CzkawkaDuplicateFinder,
    size: u64,
) {
    if !finder.is_null() {
        unsafe {
            (*finder).finder.set_minimal_file_size(size);
        }
    }
}

// Set maximum file size
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_set_max_size(
    finder: *mut CzkawkaDuplicateFinder,
    size: u64,
) {
    if !finder.is_null() {
        unsafe {
            (*finder).finder.set_maximal_file_size(size);
        }
    }
}

// Start the search
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_search(finder: *mut CzkawkaDuplicateFinder) -> bool {
    if finder.is_null() {
        return false;
    }

    unsafe {
        let finder_ptr = &mut *finder;
        finder_ptr.stop_flag.store(false, Ordering::Relaxed);

        // Set included and excluded paths using CommonData trait methods
        let included = std::mem::take(&mut finder_ptr.included_paths);
        let excluded = std::mem::take(&mut finder_ptr.excluded_paths);

        finder_ptr.finder.set_included_paths(included);
        finder_ptr.finder.set_excluded_paths(excluded);

        // Call search from the Search trait
        finder_ptr.finder.search(&finder_ptr.stop_flag, None);
        true
    }
}

// Stop the search
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_stop(finder: *mut CzkawkaDuplicateFinder) {
    if !finder.is_null() {
        unsafe {
            (*finder).stop_flag.store(true, Ordering::Relaxed);
        }
    }
}

// Get results count
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_get_group_count(
    finder: *const CzkawkaDuplicateFinder,
) -> usize {
    if finder.is_null() {
        return 0;
    }

    unsafe {
        let finder = &*finder;
        match finder.finder.get_params().check_method {
            CheckingMethod::Hash => finder.finder.get_files_sorted_by_hash().len(),
            CheckingMethod::Name => finder.finder.get_files_sorted_by_names().len(),
            CheckingMethod::Size => finder.finder.get_files_sorted_by_size().len(),
            CheckingMethod::SizeName => finder.finder.get_files_sorted_by_size_name().len(),
            _ => 0,
        }
    }
}

// Get wasted space
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_get_wasted_space(
    finder: *const CzkawkaDuplicateFinder,
) -> u64 {
    if finder.is_null() {
        return 0;
    }

    unsafe {
        let finder = &*finder;
        let info = finder.finder.get_information();
        match finder.finder.get_params().check_method {
            CheckingMethod::Hash => info.lost_space_by_hash,
            CheckingMethod::Name => 0, // Name-based doesn't track wasted space
            CheckingMethod::Size => info.lost_space_by_size,
            CheckingMethod::SizeName => 0,
            _ => 0,
        }
    }
}

// Get duplicate group by index
#[no_mangle]
pub extern "C" fn czkawka_duplicate_finder_get_group(
    finder: *const CzkawkaDuplicateFinder,
    group_index: usize,
    out_entries: *mut *const CDuplicateEntry,
    out_count: *mut usize,
) -> bool {
    if finder.is_null() || out_entries.is_null() || out_count.is_null() {
        return false;
    }

    unsafe {
        let finder = &*finder;

        match finder.finder.get_params().check_method {
            CheckingMethod::Hash => {
                let groups = finder.finder.get_files_sorted_by_hash();
                if let Some((_, group_vecs)) = groups.iter().nth(group_index) {
                    // Flatten all vectors in this group
                    let mut all_entries = Vec::new();
                    for vec in group_vecs {
                        all_entries.extend_from_slice(vec);
                    }

                    let c_entries: Vec<CDuplicateEntry> = all_entries
                        .iter()
                        .map(|entry| CDuplicateEntry {
                            path: CString::new(entry.path.to_string_lossy().to_string())
                                .unwrap()
                                .into_raw(),
                            size: entry.size,
                            modified_date: entry.modified_date,
                            hash: CString::new(entry.hash.clone()).unwrap().into_raw(),
                        })
                        .collect();

                    *out_count = c_entries.len();
                    *out_entries = Box::into_raw(c_entries.into_boxed_slice()) as *const CDuplicateEntry;
                    return true;
                }
            }
            CheckingMethod::Name => {
                let groups = finder.finder.get_files_sorted_by_names();
                if let Some((_, entries)) = groups.iter().nth(group_index) {
                    let c_entries: Vec<CDuplicateEntry> = entries
                        .iter()
                        .map(|entry| CDuplicateEntry {
                            path: CString::new(entry.path.to_string_lossy().to_string())
                                .unwrap()
                                .into_raw(),
                            size: entry.size,
                            modified_date: entry.modified_date,
                            hash: CString::new("").unwrap().into_raw(),
                        })
                        .collect();

                    *out_count = c_entries.len();
                    *out_entries = Box::into_raw(c_entries.into_boxed_slice()) as *const CDuplicateEntry;
                    return true;
                }
            }
            CheckingMethod::Size => {
                let groups = finder.finder.get_files_sorted_by_size();
                if let Some((_, entries)) = groups.iter().nth(group_index) {
                    let c_entries: Vec<CDuplicateEntry> = entries
                        .iter()
                        .map(|entry| CDuplicateEntry {
                            path: CString::new(entry.path.to_string_lossy().to_string())
                                .unwrap()
                                .into_raw(),
                            size: entry.size,
                            modified_date: entry.modified_date,
                            hash: CString::new("").unwrap().into_raw(),
                        })
                        .collect();

                    *out_count = c_entries.len();
                    *out_entries = Box::into_raw(c_entries.into_boxed_slice()) as *const CDuplicateEntry;
                    return true;
                }
            }
            CheckingMethod::SizeName => {
                let groups = finder.finder.get_files_sorted_by_size_name();
                if let Some((_, entries)) = groups.iter().nth(group_index) {
                    let c_entries: Vec<CDuplicateEntry> = entries
                        .iter()
                        .map(|entry| CDuplicateEntry {
                            path: CString::new(entry.path.to_string_lossy().to_string())
                                .unwrap()
                                .into_raw(),
                            size: entry.size,
                            modified_date: entry.modified_date,
                            hash: CString::new("").unwrap().into_raw(),
                        })
                        .collect();

                    *out_count = c_entries.len();
                    *out_entries = Box::into_raw(c_entries.into_boxed_slice()) as *const CDuplicateEntry;
                    return true;
                }
            }
            _ => {}
        }

        false
    }
}

// Free duplicate entry array
#[no_mangle]
pub extern "C" fn czkawka_duplicate_entries_free(entries: *mut CDuplicateEntry, count: usize) {
    if entries.is_null() {
        return;
    }

    unsafe {
        let entries_slice = std::slice::from_raw_parts_mut(entries, count);
        for entry in entries_slice {
            if !entry.path.is_null() {
                let _ = CString::from_raw(entry.path as *mut c_char);
            }
            if !entry.hash.is_null() {
                let _ = CString::from_raw(entry.hash as *mut c_char);
            }
        }
        let _ = Box::from_raw(std::slice::from_raw_parts_mut(entries, count));
    }
}
