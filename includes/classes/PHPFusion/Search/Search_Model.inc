<?php
/*-------------------------------------------------------+
| PHP-Fusion Content Management System
| Copyright (C) PHP-Fusion Inc
| https://www.php-fusion.co.uk/
+--------------------------------------------------------+
| Filename: Search_Model.inc
| Author: Frederick MC Chan
+--------------------------------------------------------+
| This program is released as free software under the
| Affero GPL license. You can redistribute it and/or
| modify it under the terms of this license which you
| can read by viewing the included agpl.txt or online
| at www.gnu.org/licenses/agpl.html. Removal of this
| copyright header is strictly prohibited without
| written permission from the original author(s).
+--------------------------------------------------------*/
namespace PHPFusion\Search;
/**
 * Class Search_Model
 *
 * @package PHPFusion\Search
 */
abstract class Search_Model {

    protected static $available_modules = array();
    protected static $form_config = array();

    protected static $search_result_array = [];
    protected static $site_search_count = 0;
    protected static $navigation_result = '';
    protected static $items_count = '';
    protected static $global_string_count = 0;

    protected static $memory_limit = 0;
    protected static $memory_exhausted = FALSE;

    // Query Formatting
    protected static $fields_count = 0;
    protected static $swords  = array();
    protected static $c_swords  = 0;
    protected static $i_swords  = 0;
    protected static $swords_keys_for_query = array();
    protected static $swords_values_for_query = array();
    protected static $conditions = array();
    protected static $fieldsvar = '';

    /*
     * Default values
     */
    protected static $rowstart = 0;
    protected static $search_text = '';
    protected static $search_method = 'OR';
    protected static $search_date_limit = 0;
    protected static $search_fields = 2;
    protected static $search_sort = 'datestamp';
    protected static $search_order = 0;
    protected static $search_chars = 50;
    protected static $forum_id = 0;
    protected static $search_type = 'all';
    protected static $search_param = array();
    public static $locale = array();
    protected static $composevars = '';

    protected function __construct() {
    }

    protected function init() {

        $search_modules = self::cache_modules();

        // Formats POST
        if (isset($_GET['rowstart']) && isnum($_GET['rowstart'])) {
            self::$rowstart = $_GET['rowstart'];
        }
        // Formats sText
        if (isset($_POST['stext'])) {
            if (is_array($_POST['stext'])) {
                redirect(FUSION_SELF);
            } else {
                self::$search_text = urlencode(stripinput($_POST['stext']));
            }
        } elseif (isset($_GET['stext']) && $_GET['stext']) {
            self::$search_text = urlencode(stripinput($_GET['stext']));
        }
        // Formats Search Method
        if (isset($_POST['method']) && in_array($_POST['method'], array('OR', 'AND'))) {
            self::$search_method = $_POST['method'];
        } elseif (isset($_GET['method']) && in_array($_GET['method'], array('OR', 'AND'))) {
            self::$search_method = $_GET['method'];
        }
        // Formats search date limit
        if (isset($_POST['datelimit']) && isnum($_POST['datelimit'])) {
            self::$search_date_limit = $_POST['datelimit'];
        } elseif (isset($_GET['datelimit']) && isnum($_GET['datelimit'])) {
            self::$search_date_limit = $_GET['datelimit'];
        }
        // Fields
        if (isset($_POST['fields']) && isnum($_POST['fields'])) {
            self::$search_fields = $_POST['fields'];
        } elseif (isset($_GET['fields']) && isnum($_GET['fields'])) {
            self::$search_fields = $_GET['fields'];
        }
        // Sorting
        if (isset($_POST['sort']) && in_array($_POST['sort'], array("datestamp", "subject", "author"))) {
            self::$search_sort = $_POST['sort'];
        } elseif ((isset($_GET['sort']) && in_array($_GET['sort'], array('datestamp', 'subject', 'author')))) {
            self::$search_sort = $_GET['sort'];
        }
        // Orders
        if (isset($_POST['order']) && isnum($_POST['order'])) {
            self::$search_order = $_POST['order'];
        } elseif (isset($_GET['order']) && isnum($_GET['order'])) {
            self::$search_order = $_GET['order'];
        }
        // Characters
        if (isset($_POST['chars']) && isnum($_POST['chars'])) {
            self::$search_chars = ($_POST['chars'] > 200 ? 200 : $_POST['chars']);
        } elseif (isset($_GET['chars']) && isnum($_GET['chars'])) {
            self::$search_chars = ($_GET['chars'] > 200 ? 200 : $_GET['chars']);
        }
        // Forum ID
        if (isset($_POST['forum_id']) && isnum($_POST['forum_id'])) {
            self::$forum_id = $_POST['forum_id'];
        } elseif (isset($_GET['forum_id']) && isnum($_GET['forum_id'])) {
            self::$forum_id = $_GET['forum_id'];
        }

        // Prepare SType
        if (isset($_GET['stype']) || isset($_GET['stype']) && in_array(isset($_GET['stype']), $search_modules)) {
            if (isset($_GET['stype']) && in_array($_GET['stype'], $search_modules) || isset($_POST['stype']) && in_array($_POST['stype'], $search_modules)) {
                self::$search_type = (isset($_POST['stype']) ? lcfirst($_POST['stype']) : (isset($_GET['stype']) ? lcfirst($_GET['stype']) : lcfirst(str_replace('.php', '', fusion_get_settings('default_search')))));
            }
        } else {
            self::$search_type = (isset($_POST['stype']) && in_array($_POST['stype'], $search_modules) ? $_POST['stype'] : lcfirst(str_replace('.php', '', fusion_get_settings('default_search'))));
        }

        self::$form_config = self::load_search_modules();

        // Memory Limits
        $memory_limit = str_replace("m", "", strtolower(ini_get("memory_limit"))) * 1024 * 1024;
        $memory_limit = !isnum($memory_limit) ? 8 * 1024 * 1024 : $memory_limit < 8 * 1024 * 1024 ? 8 * 1024 * 1024 : $memory_limit;
        self::$memory_limit = $memory_limit - ceil($memory_limit / 4);
    }

    protected function load_search_modules() {
        $radio_button = array();
        $form_elements = array();
        if (!empty(self::$available_modules)) {
            foreach (self::$available_modules as $module_name) {
                if ($module_name !== 'all') {
                    include(INCLUDES."search/search_".$module_name."_include_button.php");
                }
            }
        }
        sort($radio_button);
        self::$form_config = [
            'form_elements' => $form_elements,
            'radio_button'  => $radio_button,
        ];

        return self::$form_config;
    }

    protected function cache_modules() {
        if (empty(self::$available_modules)) {
            self::$available_modules = array('0' => 'all');
            $search_dir = INCLUDES."search/";
            $search_files = makefilelist($search_dir, '.|..|index.php|location.json.php|users.json.php|.DS_Store', TRUE, 'files');
            $search_files = array_flip($search_files);
            foreach ($search_files as $key => $file_to_check) {
                if (preg_match("/include_button.php/i", $key)) {
                    $file_name = str_replace('_include_button.php', '', $key);
                    $file_name = str_replace('search_', '', $file_name);
                    self::$available_modules[] = $file_name;
                }
            }
        }

        return self::$available_modules;
    }

    /*
     * Compile Search Results - HTML
     */
    public static function search_globalarray($search_result) {
        if (!empty($search_result)) {
            self::$global_string_count += strlen($search_result);
            if (self::$memory_limit > self::$global_string_count) {
                self::$search_result_array[] = $search_result;
                self::$memory_exhausted = FALSE;
            } else {
                self::$memory_exhausted = TRUE;
            }
        }
        return self::$memory_exhausted;
    }

    public static function append_item_count($value) {
        self::$items_count .= $value;
    }


    public static function search_striphtmlbbcodes($text) {
        $text = preg_replace("[\[(.*?)\]]", "", $text);
        $text = preg_replace("<\<(.*?)\>>", "", $text);
        return $text;
    }

    public static function search_textfrag($text) {
        if (Search_Engine::get_param('chars') != 0) {
            $text = nl2br(stripslashes(substr($text, 0, Search_Engine::get_param('chars'))."..."));
        } else {
            $text = nl2br(stripslashes($text));
        }
        return $text;
    }

    public static function search_stringscount($text) {
        $count = 0;
        $c_swords = self::$c_swords;
        for ($i = 0; $i < $c_swords; $i++) {
            $count += substr_count(strtolower($text), strtolower(self::$swords[$i]));
        }
        return $count;
    }

    /*
     * Indexer to avoid duplication of fields search in other module.
     */
    private static $search_index = 0;
    private static $search_mod;

    // conditions parser
    public static function search_column($field, $field_module) {
        if (self::$search_mod == $field_module) {
            self::$search_index++;
        } else {
            self::$search_mod = $field_module;
            self::$search_index = 0;
        }
        $last_sword_index = self::$c_swords - 1;
        for ($i = 0; $i <  self::$c_swords; $i++) {
            if (isset(self::$swords_keys_for_query[ $i * self::$fields_count + self::$search_index ])) {
                $sword_var = self::$swords_keys_for_query[ $i * self::$fields_count + self::$search_index ];
                self::$conditions[$field_module][$field][] = $field ." LIKE {$sword_var}" . ($i < $last_sword_index ? ' '.Search_Engine::get_param('method').' ' : '');
            }
        }
    }


    public static function search_conditions($field_module) {
        // the conditions is imposition and must reset.
        if (!empty(self::$conditions[$field_module])) {
            return "(".implode(' || ', array_map(function($field_var){ return implode('', $field_var); }, self::$conditions[$field_module])).")";
        }
    }



    // generate search navigation
    public static function search_navigation($rows) {
        self::$site_search_count += $rows;
        $navigation_result = "<div class='center m-t-5'>\n";
        $navigation_result .= makePageNav(Search_Engine::get_param('rowstart'), 10, (self::$site_search_count > 100 || self::search_globalarray("") ? 100 : self::$site_search_count), 3, BASEDIR."search.php?stype=".Search_Engine::get_param('stype')."&amp;stext=".urlencode(Search_Engine::get_param('stext'))."&amp;".Search_Engine::get_param('composevars'));
        $navigation_result .= "\n</div>\n";
        self::$navigation_result = $navigation_result;
    }
}