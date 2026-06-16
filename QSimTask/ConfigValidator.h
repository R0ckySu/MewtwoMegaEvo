//
// Created by Rocky Su & Claude Fable 5 on 16/06/2026.
//
// JSON configuration file validator for MewtwoMegaEvo simulator.
// Validates required fields, types, and external matrix file existence
// for the three core configuration files loaded during QSimTask::preload().
//

#ifndef MEWTWOMEGAEVO_CONFIGVALIDATOR_H
#define MEWTWOMEGAEVO_CONFIGVALIDATOR_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <sys/stat.h>

/**
 * @brief Validates the three JSON configuration files used by MewtwoMegaEvo.
 *
 * Usage:
 *   ConfigValidator validator(config_folder_path);
 *   validator.validate_sim_config(sim_configs_json);
 *   validator.validate_gate_config(gate_configs_json);
 *   validator.validate_hamiltonian_config(hamiltonian_configs_json);
 *
 * Validation errors are accumulated and can be retrieved via getErrors().
 * Warnings (non-fatal) are also collected.
 */
class ConfigValidator {
public:
    /**
     * @param config_folder Absolute or relative path where config JSON files
     *                      and external matrix/data files reside.
     */
    explicit ConfigValidator(std::string config_folder);
    ~ConfigValidator();

    // ── Per-file validation ──────────────────────────────────────────

    /** Validate sim_config.json contents. */
    bool validate_sim_config(const nlohmann::json& json);

    /** Validate gate_config.json contents. */
    bool validate_gate_config(const nlohmann::json& json);

    /** Validate hamiltonian_config.json contents. */
    bool validate_hamiltonian_config(const nlohmann::json& json);

    // ── Results ──────────────────────────────────────────────────────

    /** True when NO errors were found across all validated files. */
    bool is_valid() const;

    /** Accumulated error messages (blocking issues). */
    const std::vector<std::string>& get_errors() const;

    /** Accumulated warning messages (non-blocking). */
    const std::vector<std::string>& get_warnings() const;

    /** Print a formatted validation report to stderr. */
    void print_report() const;

    /**
     * If validation errors exist, throw a std::runtime_error containing
     * the full validation report. Warnings alone do NOT trigger a throw.
     * Call this after all validate_* methods to halt on fatal config issues.
     */
    void throw_on_error() const;

private:
    std::string config_folder_;
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;

    // ── Helpers ──────────────────────────────────────────────────────

    /** Check if a JSON key exists in an object; record error if missing. */
    bool check_required_key(const nlohmann::json& obj,
                            const std::string& key,
                            const std::string& context);

    /** Check required keys as a list. Returns false if any are missing. */
    bool check_required_keys(const nlohmann::json& obj,
                             const std::vector<std::string>& keys,
                             const std::string& context);

    /**
     * Determine if a symbolic name references a matrix file on disk
     * (as opposed to a built-in Pauli-string like "IX", "iXYZ", etc.).
     * If it IS a file reference, verify the file exists.
     *
     * @param symbol_name  e.g. "IX", "rho0", "HmwRWASbNMRAmp"
     * @param context      human-readable tag for error messages
     * @return true if valid (either a recognised Pauli string or an
     *         existing file)
     */
    bool verify_symbolic_matrix(const std::string& symbol_name,
                                const std::string& context);

    /** Check that a file path (relative or absolute) exists. */
    bool file_exists(const std::string& path) const;

    /** Check a waveform_path field: empty-string is permitted;
     *  non-empty must point to an existing file (with '#' wildcard
     *  expansion handled). */
    bool verify_waveform_path(const std::string& path,
                              const std::string& context);

    /** Check an ext_shaped_sig_path: empty is OK, non-empty must exist. */
    bool verify_ext_shaped_sig_path(const std::string& path,
                                    const std::string& context);

    /**
     * Build a human-readable context string including the item's "tag"
     * field when available.  Produces:
     *   "file: array_name[3] (tag=\"MyGate\")"  or  "file: array_name[3]"
     * if the tag is missing.
     */
    static std::string item_context(const std::string& file,
                                    const std::string& array_name,
                                    size_t index,
                                    const nlohmann::json& item);
};

#endif // MEWTWOMEGAEVO_CONFIGVALIDATOR_H
