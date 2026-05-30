-- AngelDB - Sample update file
-- Place your .sql files in the pending/ directory.
-- They will be executed in alphabetical order when AngelScript loads/reloads.
-- Successfully executed files are moved to applied/.
-- Failed files remain in pending/ (check server logs for errors).

-- Example: Create a table for custom spawn data
CREATE TABLE IF NOT EXISTS as_custom_spawns (
    id       INT AUTO_INCREMENT PRIMARY KEY,
    entry    INT UNSIGNED NOT NULL,
    map      INT UNSIGNED NOT NULL DEFAULT 0,
    pos_x    FLOAT NOT NULL DEFAULT 0,
    pos_y    FLOAT NOT NULL DEFAULT 0,
    pos_z    FLOAT NOT NULL DEFAULT 0,
    orient   FLOAT NOT NULL DEFAULT 0,
    name     VARCHAR(255) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Example: Create a table for custom config values
CREATE TABLE IF NOT EXISTS as_custom_config (
    key_name   VARCHAR(128) NOT NULL PRIMARY KEY,
    value      TEXT NOT NULL,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Example: Insert default config values
INSERT IGNORE INTO as_custom_config (key_name, value) VALUES
    ('server_name', 'AngelCore Custom'),
    ('max_custom_spawns', '1000');
