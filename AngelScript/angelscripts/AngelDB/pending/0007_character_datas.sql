-- Character Data Table
-- Stores additional character data that shouldn't modify TrinityCore's core tables
CREATE TABLE IF NOT EXISTS character_datas (
    guid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    rpe_login TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_rpe_login (rpe_login)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
