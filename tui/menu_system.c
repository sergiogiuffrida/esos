/**
 * @file menu_system.c
 * @brief Contains the menu actions for the 'System' menu.
 * @author Copyright (c) 2019 Quantum Corporation
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <iniparser.h>
#include <cdk.h>
#include <string.h>
#include <cdk/swindow.h>
#include <sys/time.h>
#include <assert.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"


/**
 * @brief Run the "Networking" dialog.
 */
void networkDialog(CDKSCREEN *main_cdk_screen) {
    CDKSCREEN *net_screen = 0;
    CDKLABEL *net_label = 0, *short_label = 0;
    CDKENTRY *host_name = 0, *domain_name = 0, *default_gw = 0,
            *name_server_1 = 0, *name_server_2 = 0, *name_server_3 = 0,
            *ip_addy = 0, *netmask = 0, *broadcast = 0, *iface_mtu = 0;
    CDKMENTRY *bond_opts = 0, *ethtool_opts = 0, *vlan_egress_map = 0;
    CDKRADIO *ip_config = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKSELECTION *slave_select = 0, *br_member_select = 0;
    WINDOW *net_window = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    dictionary *ini_dict = NULL;
    FILE *ini_file = NULL;
    int i = 0, window_y = 0, window_x = 0,
            traverse_ret = 0, net_window_lines = 0, net_window_cols = 0,
            poten_slave_cnt = 0, slaves_line_size = 0, slave_val_size = 0,
            poten_br_member_cnt = 0, br_members_line_size = 0,
            br_member_val_size = 0, temp_int = 0;
    char *net_info_msg[MAX_NET_INFO_LINES] = {NULL},
            *short_label_msg[NET_SHORT_INFO_LINES] = {NULL},
            *poten_slaves[MAX_NET_IFACE] = {NULL},
            *poten_br_members[MAX_NET_IFACE] = {NULL};
    char *error_msg = NULL, *temp_pstr = NULL, *strtok_result = NULL;
    const char *conf_hostname = NULL, *conf_domainname = NULL,
            *conf_defaultgw = NULL, *conf_nameserver1 = NULL,
            *conf_nameserver2 = NULL, *conf_nameserver3 = NULL,
            *conf_bootproto = NULL, *conf_ipaddr = NULL, *conf_netmask = NULL,
            *conf_broadcast = NULL, *conf_if_mtu = NULL, *conf_slaves = NULL,
            *conf_brmembers = NULL, *conf_bondopts = NULL,
            *conf_ethtoolopts = NULL, *conf_vlanegressmap = NULL;
    char net_if_name[MISC_STRING_LEN] = {0}, net_if_mac[MISC_STRING_LEN] = {0},
            net_if_speed[MISC_STRING_LEN] = {0},
            net_if_duplex[MISC_STRING_LEN] = {0},
            temp_ini_str[MAX_INI_VAL] = {0},
            slaves_list_line_buffer[MAX_SLAVES_LIST_BUFF] = {0},
            br_members_list_line_buffer[MAX_BR_MEMBERS_LIST_BUFF] = {0},
            bond_opts_buffer[MAX_BOND_OPTS_BUFF] = {0},
            ethtool_opts_buffer[MAX_ETHTOOL_OPTS_BUFF] = {0},
            vlan_egress_map_buffer[MAX_VLAN_EGRESS_MAP_BUFF] = {0};
    bonding_t net_if_bonding = {0};
    boolean general_opt = FALSE, question = FALSE, net_if_bridge = FALSE;

    /* Have the user select a network configuration option */
    getNetConfChoice(main_cdk_screen, &general_opt, net_if_name, net_if_mac,
            net_if_speed, net_if_duplex, &net_if_bonding, &net_if_bridge,
            poten_slaves, &poten_slave_cnt,
            poten_br_members, &poten_br_member_cnt);
    if ((general_opt == FALSE) && (net_if_name[0] == '\0'))
        return;

    while (1) {
        /* Present the "General Network Settings" screen of widgets */
        if (general_opt) {
            /* Setup a new CDK screen */
            net_window_lines = 16;
            net_window_cols = 68;
            window_y = ((LINES / 2) - (net_window_lines / 2));
            window_x = ((COLS / 2) - (net_window_cols / 2));
            net_window = newwin(net_window_lines, net_window_cols,
                    window_y, window_x);
            if (net_window == NULL) {
                errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
                break;
            }
            net_screen = initCDKScreen(net_window);
            if (net_screen == NULL) {
                errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
                break;
            }
            boxWindow(net_window, g_color_dialog_box[g_curr_theme]);
            wbkgd(net_window, g_color_dialog_text[g_curr_theme]);
            wrefresh(net_window);

            SAFE_ASPRINTF(&net_info_msg[0],
                    "</%d/B>General network settings...",
                    g_color_dialog_title[g_curr_theme]);
            SAFE_ASPRINTF(&net_info_msg[1], " ");

            /* Read network configuration file (INI file) */
            ini_dict = iniparser_load(NETWORK_CONF);
            if (ini_dict == NULL) {
                errorDialog(main_cdk_screen, NET_CONF_READ_ERR, NULL);
                return;
            }
            conf_hostname = iniparser_getstring(ini_dict,
                    "general:hostname", "");
            conf_domainname = iniparser_getstring(ini_dict,
                    "general:domainname", "");
            conf_defaultgw = iniparser_getstring(ini_dict,
                    "general:defaultgw", "");
            conf_nameserver1 = iniparser_getstring(ini_dict,
                    "general:nameserver1", "");
            conf_nameserver2 = iniparser_getstring(ini_dict,
                    "general:nameserver2", "");
            conf_nameserver3 = iniparser_getstring(ini_dict,
                    "general:nameserver3", "");

            /* Information label */
            net_label = newCDKLabel(net_screen, (window_x + 1), (window_y + 1),
                    net_info_msg, 2, FALSE, FALSE);
            if (!net_label) {
                errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
                break;
            }
            setCDKLabelBackgroundAttrib(net_label,
                    g_color_dialog_text[g_curr_theme]);

            /* Host name field */
            host_name = newCDKEntry(net_screen, (window_x + 1), (window_y + 3),
                    NULL, "</B>System host name: ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    MAX_HOSTNAME, 0, MAX_HOSTNAME, FALSE, FALSE);
            if (!host_name) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(host_name,
                    g_color_dialog_input[g_curr_theme]);
            setCDKEntryBackgroundAttrib(host_name,
                    g_color_dialog_text[g_curr_theme]);
            setCDKEntryValue(host_name, conf_hostname);

            /* Domain name field */
            domain_name = newCDKEntry(net_screen, (window_x + 1),
                    (window_y + 5), NULL, "</B>System domain name: ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    MAX_DOMAINNAME, 0, MAX_DOMAINNAME, FALSE, FALSE);
            if (!domain_name) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(domain_name,
                    g_color_dialog_input[g_curr_theme]);
            setCDKEntryBackgroundAttrib(domain_name,
                    g_color_dialog_text[g_curr_theme]);
            setCDKEntryValue(domain_name, conf_domainname);

            /* Default gateway field */
            default_gw = newCDKEntry(net_screen, (window_x + 1), (window_y + 7),
                    NULL, "</B>Default gateway (leave blank if using DHCP): ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
            if (!default_gw) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(default_gw,
                    g_color_dialog_input[g_curr_theme]);
            setCDKEntryBackgroundAttrib(default_gw,
                    g_color_dialog_text[g_curr_theme]);
            setCDKEntryValue(default_gw, conf_defaultgw);

            /* A very small label for instructions */
            SAFE_ASPRINTF(&short_label_msg[0],
                    "</B>Name Servers (leave blank if using DHCP)");
            short_label = newCDKLabel(net_screen,
                    (window_x + 1), (window_y + 9),
                    short_label_msg, NET_SHORT_INFO_LINES, FALSE, FALSE);
            if (!short_label) {
                errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
                break;
            }
            setCDKLabelBackgroundAttrib(short_label,
                    g_color_dialog_text[g_curr_theme]);

            /* Primary name server field */
            name_server_1 = newCDKEntry(net_screen, (window_x + 1),
                    (window_y + 10), NULL, "</B>NS 1: ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
            if (!name_server_1) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(name_server_1,
                    g_color_dialog_input[g_curr_theme]);
            setCDKEntryBackgroundAttrib(name_server_1,
                    g_color_dialog_text[g_curr_theme]);
            setCDKEntryValue(name_server_1, conf_nameserver1);

            /* Secondary name server field */
            name_server_2 = newCDKEntry(net_screen, (window_x + 1),
                    (window_y + 11), NULL, "</B>NS 2: ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
            if (!name_server_2) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(name_server_2,
                    g_color_dialog_input[g_curr_theme]);
            setCDKEntryBackgroundAttrib(name_server_2,
                    g_color_dialog_text[g_curr_theme]);
            setCDKEntryValue(name_server_2, conf_nameserver2);

            /* Tertiary name server field */
            name_server_3 = newCDKEntry(net_screen, (window_x + 1),
                    (window_y + 12), NULL, "</B>NS 3: ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
            if (!name_server_3) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(name_server_3,
                    g_color_dialog_input[g_curr_theme]);
            setCDKEntryBackgroundAttrib(name_server_3,
                    g_color_dialog_text[g_curr_theme]);
            setCDKEntryValue(name_server_3, conf_nameserver3);

            /* Buttons */
            ok_button = newCDKButton(net_screen, (window_x + 26),
                    (window_y + 14), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
            if (!ok_button) {
                errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
                break;
            }
            setCDKButtonBackgroundAttrib(ok_button,
                    g_color_dialog_input[g_curr_theme]);
            cancel_button = newCDKButton(net_screen, (window_x + 36),
                    (window_y + 14), g_ok_cancel_msg[1], cancel_cb,
                    FALSE, FALSE);
            if (!cancel_button) {
                errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
                break;
            }
            setCDKButtonBackgroundAttrib(cancel_button,
                    g_color_dialog_input[g_curr_theme]);

            /* Allow user to traverse the screen */
            refreshCDKScreen(net_screen);
            traverse_ret = traverseCDKScreen(net_screen);

            /* User hit 'OK' button */
            if (traverse_ret == 1) {
                /* Turn the cursor off (pretty) */
                curs_set(0);

                /* Check the host name value (field entry)  -- required */
                if (!checkInputStr(main_cdk_screen, NAME_CHARS,
                        getCDKEntryValue(host_name))) {
                    traverse_ret = 0; /* Skip the prompt */
                    break;
                }

                /* Check the domain name value (field entry)  -- required */
                if (!checkInputStr(main_cdk_screen, NAME_CHARS,
                        getCDKEntryValue(domain_name))) {
                    traverse_ret = 0; /* Skip the prompt */
                    break;
                }

                /* Check the default gateway value (field entry) */
                if (strlen(getCDKEntryValue(default_gw)) != 0) {
                    if (!checkInputStr(main_cdk_screen, IPADDR_CHARS,
                            getCDKEntryValue(default_gw))) {
                        traverse_ret = 0; /* Skip the prompt */
                        break;
                    }
                }

                /* Check the name server (1) value (field entry) */
                if (strlen(getCDKEntryValue(name_server_1)) != 0) {
                    if (!checkInputStr(main_cdk_screen, IPADDR_CHARS,
                            getCDKEntryValue(name_server_1))) {
                        traverse_ret = 0; /* Skip the prompt */
                        break;
                    }
                }

                /* Check the name server (2) value (field entry) */
                if (strlen(getCDKEntryValue(name_server_2)) != 0) {
                    if (!checkInputStr(main_cdk_screen, IPADDR_CHARS,
                            getCDKEntryValue(name_server_2))) {
                        traverse_ret = 0; /* Skip the prompt */
                        break;
                    }
                }

                /* Check the name server (3) value (field entry) */
                if (strlen(getCDKEntryValue(name_server_3)) != 0) {
                    if (!checkInputStr(main_cdk_screen, IPADDR_CHARS,
                            getCDKEntryValue(name_server_3))) {
                        traverse_ret = 0; /* Skip the prompt */
                        break;
                    }
                }

                /* Write to network config. file */
                if (iniparser_set(ini_dict, "general", NULL) == -1) {
                    errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                    break;
                }
                if (iniparser_set(ini_dict, "general:hostname",
                        getCDKEntryValue(host_name)) == -1) {
                    errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                    break;
                }
                if (iniparser_set(ini_dict, "general:domainname",
                        getCDKEntryValue(domain_name)) == -1) {
                    errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                    break;
                }
                if (iniparser_set(ini_dict, "general:defaultgw",
                        getCDKEntryValue(default_gw)) == -1) {
                    errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                    break;
                }
                if (iniparser_set(ini_dict, "general:nameserver1",
                        getCDKEntryValue(name_server_1)) == -1) {
                    errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                    break;
                }
                if (iniparser_set(ini_dict, "general:nameserver2",
                        getCDKEntryValue(name_server_2)) == -1) {
                    errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                    break;
                }
                if (iniparser_set(ini_dict, "general:nameserver3",
                        getCDKEntryValue(name_server_3)) == -1) {
                    errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                    break;
                }
                ini_file = fopen(NETWORK_CONF, "w");
                if (ini_file == NULL) {
                    SAFE_ASPRINTF(&error_msg, NET_CONF_WRITE_ERR,
                            strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                } else {
                    fprintf(ini_file, NET_CONF_COMMENT_1);
                    fprintf(ini_file, INI_CONF_COMMENT);
                    iniparser_dump_ini(ini_dict, ini_file);
                    fclose(ini_file);
                    iniparser_freedict(ini_dict);
                }
            }

            /* Present the screen for a specific
             * network interface configuration */
        } else {
            /* If an interface is enslaved, there is nothing to configure */
            if (net_if_bonding == SLAVE) {
                SAFE_ASPRINTF(&error_msg,
                        "Interface '%s' is currently enslaved to a master",
                        net_if_name);
                errorDialog(main_cdk_screen, error_msg,
                        "bonding interface, so there is nothing to configure.");
                FREE_NULL(error_msg);
                break;
            }

            /* Setup a new CDK screen */
            net_window_lines = 18;
            net_window_cols = 70;
            window_y = ((LINES / 2) - (net_window_lines / 2));
            window_x = ((COLS / 2) - (net_window_cols / 2));
            net_window = newwin(net_window_lines, net_window_cols,
                    window_y, window_x);
            if (net_window == NULL) {
                errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
                break;
            }
            net_screen = initCDKScreen(net_window);
            if (net_screen == NULL) {
                errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
                break;
            }
            boxWindow(net_window, g_color_dialog_box[g_curr_theme]);
            wbkgd(net_window, g_color_dialog_text[g_curr_theme]);
            wrefresh(net_window);

            /* Make a nice, informational label */
            SAFE_ASPRINTF(&net_info_msg[0],
                    "</%d/B>Configuring interface %s...",
                    g_color_dialog_title[g_curr_theme], net_if_name);
            SAFE_ASPRINTF(&net_info_msg[1], " ");
            SAFE_ASPRINTF(&net_info_msg[2], "</B>MAC Address:<!B>\t%s",
                    net_if_mac);
            if (net_if_speed[0] == '\0')
                SAFE_ASPRINTF(&net_info_msg[3], "</B>Link Status:<!B>\tNone");
            else
                SAFE_ASPRINTF(&net_info_msg[3], "</B>Link Status:<!B>\t%s, %s",
                    net_if_speed,
                    net_if_duplex);
            SAFE_ASPRINTF(&net_info_msg[4], "</B>Bonding:<!B>\t%s",
                    g_bonding_map[net_if_bonding]);

            /* Read network configuration file (INI file) */
            ini_dict = iniparser_load(NETWORK_CONF);
            if (ini_dict == NULL) {
                errorDialog(main_cdk_screen, NET_CONF_READ_ERR, NULL);
                break;
            }
            snprintf(temp_ini_str, MAX_INI_VAL, "%s:bootproto",
                    net_if_name);
            conf_bootproto = iniparser_getstring(ini_dict, temp_ini_str, "");
            snprintf(temp_ini_str, MAX_INI_VAL, "%s:ipaddr",
                    net_if_name);
            conf_ipaddr = iniparser_getstring(ini_dict, temp_ini_str, "");
            snprintf(temp_ini_str, MAX_INI_VAL, "%s:netmask",
                    net_if_name);
            conf_netmask = iniparser_getstring(ini_dict, temp_ini_str, "");
            snprintf(temp_ini_str, MAX_INI_VAL, "%s:broadcast",
                    net_if_name);
            conf_broadcast = iniparser_getstring(ini_dict, temp_ini_str, "");
            snprintf(temp_ini_str, MAX_INI_VAL, "%s:slaves",
                    net_if_name);
            conf_slaves = iniparser_getstring(ini_dict, temp_ini_str, "");
            snprintf(temp_ini_str, MAX_INI_VAL, "%s:brmembers",
                    net_if_name);
            conf_brmembers = iniparser_getstring(ini_dict, temp_ini_str, "");
            snprintf(temp_ini_str, MAX_INI_VAL, "%s:bondopts",
                    net_if_name);
            conf_bondopts = iniparser_getstring(ini_dict, temp_ini_str, "");
            snprintf(temp_ini_str, MAX_INI_VAL, "%s:ethtoolopts",
                    net_if_name);
            conf_ethtoolopts = iniparser_getstring(ini_dict, temp_ini_str, "");
            snprintf(temp_ini_str, MAX_INI_VAL, "%s:vlanegressmap",
                    net_if_name);
            conf_vlanegressmap = iniparser_getstring(ini_dict, temp_ini_str,
                    "");

            /* If value doesn't exist, use a default MTU based on the
             * interface type */
            snprintf(temp_ini_str, MAX_INI_VAL, "%s:mtu", net_if_name);
            if (strstr(net_if_name, "eth") != NULL)
                conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str,
                    DEFAULT_ETH_MTU);
            else if (strstr(net_if_name, "ib") != NULL)
                conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str,
                    DEFAULT_IB_MTU);
            else if (strstr(net_if_name, "bond") != NULL)
                conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str,
                    DEFAULT_ETH_MTU);
            else if (strstr(net_if_name, "br") != NULL)
                conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str,
                    DEFAULT_ETH_MTU);
            else if (strstr(net_if_name, "enp") != NULL)
                conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str,
                    DEFAULT_ETH_MTU);
            else if (strstr(net_if_name, "eno") != NULL)
                conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str,
                    DEFAULT_ETH_MTU);
            else
                conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str,
                    DEFAULT_ETH_MTU);

            /* Information label */
            net_label = newCDKLabel(net_screen, (window_x + 1), (window_y + 1),
                    net_info_msg, 5, FALSE, FALSE);
            if (!net_label) {
                errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
                break;
            }
            setCDKLabelBackgroundAttrib(net_label,
                    g_color_dialog_text[g_curr_theme]);

            /* Interface MTU field */
            iface_mtu = newCDKEntry(net_screen, (window_x + 50), (window_y + 3),
                    "</B>Interface MTU:  ", NULL,
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vINT,
                    15, 0, 15, FALSE, FALSE);
            if (!iface_mtu) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(iface_mtu,
                    g_color_dialog_input[g_curr_theme]);
            setCDKEntryBackgroundAttrib(iface_mtu,
                    g_color_dialog_text[g_curr_theme]);
            setCDKEntryValue(iface_mtu, conf_if_mtu);

            /* IP settings radio */
            ip_config = newCDKRadio(net_screen, (window_x + 1), (window_y + 7),
                    NONE, 5, 10, "</B>IP Settings:", g_ip_opts, 3,
                    '#' | g_color_dialog_select[g_curr_theme], 1,
                    g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!ip_config) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(ip_config,
                    g_color_dialog_text[g_curr_theme]);
            if (strcasecmp(conf_bootproto, g_ip_opts[1]) == 0)
                setCDKRadioCurrentItem(ip_config, 1);
            else if (strcasecmp(conf_bootproto, g_ip_opts[2]) == 0)
                setCDKRadioCurrentItem(ip_config, 2);
            else
                setCDKRadioCurrentItem(ip_config, 0);

            /* A very small label for instructions */
            SAFE_ASPRINTF(&short_label_msg[0],
                    "</B>Static IPv4 Settings (leave blank if using DHCP)");
            short_label = newCDKLabel(net_screen,
                    (window_x + 20), (window_y + 7),
                    short_label_msg, NET_SHORT_INFO_LINES, FALSE, FALSE);
            if (!short_label) {
                errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
                break;
            }
            setCDKLabelBackgroundAttrib(short_label,
                    g_color_dialog_text[g_curr_theme]);

            /* IP address field */
            ip_addy = newCDKEntry(net_screen, (window_x + 20), (window_y + 8),
                    NULL, "</B>IP address: ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
            if (!ip_addy) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(ip_addy,
                    g_color_dialog_input[g_curr_theme]);
            setCDKEntryBackgroundAttrib(ip_addy,
                    g_color_dialog_text[g_curr_theme]);
            setCDKEntryValue(ip_addy, conf_ipaddr);

            /* Netmask field */
            netmask = newCDKEntry(net_screen, (window_x + 20), (window_y + 9),
                    NULL, "</B>Netmask:    ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
            if (!netmask) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(netmask,
                    g_color_dialog_input[g_curr_theme]);
            setCDKEntryBackgroundAttrib(netmask,
                    g_color_dialog_text[g_curr_theme]);
            setCDKEntryValue(netmask, conf_netmask);

            /* Broadcast field */
            broadcast = newCDKEntry(net_screen, (window_x + 20),
                    (window_y + 10), NULL, "</B>Broadcast:  ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
            if (!broadcast) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(broadcast,
                    g_color_dialog_input[g_curr_theme]);
            setCDKEntryBackgroundAttrib(broadcast,
                    g_color_dialog_text[g_curr_theme]);
            setCDKEntryValue(broadcast, conf_broadcast);

            // TODO: For now, bridging and bonding are mutually exclusive.
            if (net_if_bonding == MASTER) {
                /* If the interface is a master (bonding)
                 * they can set options */
                bond_opts = newCDKMentry(net_screen, (window_x + 1),
                        (window_y + 11), "</B>Bonding Options:", NULL,
                        g_color_dialog_select[g_curr_theme],
                        '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                        25, 2, 50, 0, TRUE, FALSE);
                if (!bond_opts) {
                    errorDialog(main_cdk_screen, MENTRY_ERR_MSG, NULL);
                    break;
                }
                // TODO: Some tweaking to make this widget look like the
                // others; CDK bug?
                setCDKMentryBoxAttribute(bond_opts,
                        g_color_mentry_box[g_curr_theme]);
                setCDKMentryBackgroundAttrib(bond_opts,
                        g_color_dialog_text[g_curr_theme]);
                setCDKMentryValue(bond_opts, conf_bondopts);
                /* They can also select slaves for the bond interface */
                slave_select = newCDKSelection(net_screen, (window_x + 35),
                        (window_y + 12), RIGHT, 3, 20, "</B>Bonding Slaves:",
                        poten_slaves, poten_slave_cnt,
                        g_choice_char, 2, g_color_dialog_select[g_curr_theme],
                        FALSE, FALSE);
                if (!slave_select) {
                    errorDialog(main_cdk_screen, SELECTION_ERR_MSG, NULL);
                    break;
                }
                setCDKSelectionBackgroundAttrib(slave_select,
                        g_color_dialog_text[g_curr_theme]);
                /* Parse the existing slaves (if any) */
                strtok_result = strtok(conf_slaves, ",");
                while (strtok_result != NULL) {
                    for (i = 0; i < poten_slave_cnt; i++) {
                        if (strstr(strStrip(strtok_result), poten_slaves[i]))
                            setCDKSelectionChoice(slave_select, i, 1);
                    }
                    strtok_result = strtok(NULL, ",");
                }

            } else if (net_if_bridge == TRUE) {
                /* If the interface is a bridge interface then they
                 * can select members */
                br_member_select = newCDKSelection(net_screen, (window_x + 1),
                        (window_y + 12), RIGHT, 3, 20, "</B>Bridge Members:",
                        poten_br_members, poten_br_member_cnt,
                        g_choice_char, 2, g_color_dialog_select[g_curr_theme],
                        FALSE, FALSE);
                if (!br_member_select) {
                    errorDialog(main_cdk_screen, SELECTION_ERR_MSG, NULL);
                    break;
                }
                setCDKSelectionBackgroundAttrib(br_member_select,
                        g_color_dialog_text[g_curr_theme]);
                /* Parse the existing members (if any) */
                strtok_result = strtok(conf_brmembers, ",");
                while (strtok_result != NULL) {
                    for (i = 0; i < poten_br_member_cnt; i++) {
                        if (strstr(strStrip(strtok_result),
                                poten_br_members[i]))
                            setCDKSelectionChoice(br_member_select, i, 1);
                    }
                    strtok_result = strtok(NULL, ",");
                }

            } else {
                /* If its a "normal" interface (standalone, bridge member,
                 or bonding slave) then we can set ethtool options */
                ethtool_opts = newCDKMentry(net_screen, (window_x + 1),
                        (window_y + 11), "</B>Options for 'ethtool':", NULL,
                        g_color_dialog_select[g_curr_theme],
                        '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                        25, 2, 50, 0, TRUE, FALSE);
                if (!ethtool_opts) {
                    errorDialog(main_cdk_screen, MENTRY_ERR_MSG, NULL);
                    break;
                }
                // TODO: Some tweaking to make this widget look like the
                // others; CDK bug?
                setCDKMentryBoxAttribute(ethtool_opts,
                        g_color_mentry_box[g_curr_theme]);
                setCDKMentryBackgroundAttrib(ethtool_opts,
                        g_color_dialog_text[g_curr_theme]);
                setCDKMentryValue(ethtool_opts, conf_ethtoolopts);
                /* And the VLAN egress priority map */
                vlan_egress_map = newCDKMentry(net_screen, (window_x + 30),
                        (window_y + 11), "</B>VLAN Egress Priority Map:", NULL,
                        g_color_dialog_select[g_curr_theme],
                        '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                        25, 2, 50, 0, TRUE, FALSE);
                if (!vlan_egress_map) {
                    errorDialog(main_cdk_screen, MENTRY_ERR_MSG, NULL);
                    break;
                }
                // TODO: Some tweaking to make this widget look like the
                // others; CDK bug?
                setCDKMentryBoxAttribute(vlan_egress_map,
                        g_color_mentry_box[g_curr_theme]);
                setCDKMentryBackgroundAttrib(vlan_egress_map,
                        g_color_dialog_text[g_curr_theme]);
                setCDKMentryValue(vlan_egress_map, conf_vlanegressmap);
            }

            /* Buttons */
            ok_button = newCDKButton(net_screen, (window_x + 26),
                    (window_y + 16), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
            if (!ok_button) {
                errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
                break;
            }
            setCDKButtonBackgroundAttrib(ok_button,
                    g_color_dialog_input[g_curr_theme]);
            cancel_button = newCDKButton(net_screen, (window_x + 36),
                    (window_y + 16), g_ok_cancel_msg[1], cancel_cb,
                    FALSE, FALSE);
            if (!cancel_button) {
                errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
                break;
            }
            setCDKButtonBackgroundAttrib(cancel_button,
                    g_color_dialog_input[g_curr_theme]);

            /* Allow user to traverse the screen */
            refreshCDKScreen(net_screen);
            traverse_ret = traverseCDKScreen(net_screen);

            /* User hit 'OK' button */
            if (traverse_ret == 1) {
                /* Turn the cursor off (pretty) */
                curs_set(0);

                /* Check for an MTU value if its a DHCP IP configuration */
                if (getCDKRadioCurrentItem(ip_config) == 2) {
                    /* Check the interface MTU value (field entry) */
                    if (!checkInputStr(main_cdk_screen, ASCII_CHARS,
                            getCDKEntryValue(iface_mtu))) {
                        traverse_ret = 0; /* Skip the prompt */
                        break;
                    }
                }

                /* Check all fields if its a static IP configuration */
                if (getCDKRadioCurrentItem(ip_config) == 1) {
                    /* Check the interface MTU value (field entry) */
                    if (!checkInputStr(main_cdk_screen, ASCII_CHARS,
                            getCDKEntryValue(iface_mtu))) {
                        traverse_ret = 0; /* Skip the prompt */
                        break;
                    }
                    /* Check the IP address value (field entry) */
                    if (!checkInputStr(main_cdk_screen, IPADDR_CHARS,
                            getCDKEntryValue(ip_addy))) {
                        traverse_ret = 0; /* Skip the prompt */
                        break;
                    }
                    /* Check the netmask value (field entry) */
                    if (!checkInputStr(main_cdk_screen, IPADDR_CHARS,
                            getCDKEntryValue(netmask))) {
                        traverse_ret = 0; /* Skip the prompt */
                        break;
                    }
                    /* Check the broadcast value (field entry) */
                    if (!checkInputStr(main_cdk_screen, IPADDR_CHARS,
                            getCDKEntryValue(broadcast))) {
                        traverse_ret = 0; /* Skip the prompt */
                        break;
                    }
                }

                if (getCDKRadioCurrentItem(ip_config) == 0) {
                    /* If the user sets the interface to disabled/unconfigured
                     * then we remove the section */
                    iniparser_unset(ini_dict, net_if_name);

                } else {
                    /* Network interface should be
                     * configured (static or DHCP) */
                    if (iniparser_set(ini_dict, net_if_name, NULL) == -1) {
                        errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                        break;
                    }
                    snprintf(temp_ini_str, MAX_INI_VAL,
                            "%s:bootproto", net_if_name);
                    if (iniparser_set(ini_dict, temp_ini_str,
                            g_ip_opts[getCDKRadioCurrentItem(
                            ip_config)]) == -1) {
                        errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                        break;
                    }
                    snprintf(temp_ini_str, MAX_INI_VAL, "%s:ipaddr",
                            net_if_name);
                    if (iniparser_set(ini_dict, temp_ini_str,
                            getCDKEntryValue(ip_addy)) == -1) {
                        errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                        break;
                    }
                    snprintf(temp_ini_str, MAX_INI_VAL, "%s:netmask",
                            net_if_name);
                    if (iniparser_set(ini_dict, temp_ini_str,
                            getCDKEntryValue(netmask)) == -1) {
                        errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                        break;
                    }
                    snprintf(temp_ini_str, MAX_INI_VAL,
                            "%s:broadcast", net_if_name);
                    if (iniparser_set(ini_dict, temp_ini_str,
                            getCDKEntryValue(broadcast)) == -1) {
                        errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                        break;
                    }
                    snprintf(temp_ini_str, MAX_INI_VAL, "%s:mtu",
                            net_if_name);
                    if (iniparser_set(ini_dict, temp_ini_str,
                            getCDKEntryValue(iface_mtu)) == -1) {
                        errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                        break;
                    }
                }

                if (net_if_bonding == MASTER) {
                    /* If its a bonding master, store bonding options */
                    snprintf(bond_opts_buffer, MAX_BOND_OPTS_BUFF, "%s",
                            getCDKMentryValue(bond_opts));
                    snprintf(temp_ini_str, MAX_INI_VAL, "%s:bondopts",
                            net_if_name);
                    if (iniparser_set(ini_dict, temp_ini_str,
                            bond_opts_buffer) == -1) {
                        errorDialog(main_cdk_screen,
                                "Couldn't set configuration file value!", NULL);
                        break;
                    }

                    /* For master interfaces, we need to check if any slave
                     * interfaces were selected (or removed) */
                    for (i = 0; i < poten_slave_cnt; i++) {
                        if (slave_select->selections[i] == 1) {
                            SAFE_ASPRINTF(&temp_pstr, "%s,", poten_slaves[i]);
                            /* We add one extra for the null byte */
                            slave_val_size = strlen(temp_pstr) + 1;
                            slaves_line_size =
                                    slaves_line_size + slave_val_size;
                            if (slaves_line_size >= MAX_SLAVES_LIST_BUFF) {
                                errorDialog(main_cdk_screen, "The maximum "
                                        "slaves list line buffer size "
                                        "has been reached!", NULL);
                                FREE_NULL(temp_pstr);
                                break;
                            } else {
                                strcat(slaves_list_line_buffer, temp_pstr);
                                FREE_NULL(temp_pstr);
                            }
                            /* Remove the slave interface sections
                             * from the INI file */
                            iniparser_unset(ini_dict, poten_slaves[i]);
                        }
                    }
                    /* Remove the trailing comma (if any) */
                    if (slaves_list_line_buffer[0] != '\0') {
                        temp_int = strlen(slaves_list_line_buffer) - 1;
                        if (slaves_list_line_buffer[temp_int] == ',')
                            slaves_list_line_buffer[temp_int] = '\0';
                    }
                    snprintf(temp_ini_str, MAX_INI_VAL, "%s:slaves",
                            net_if_name);
                    if (iniparser_set(ini_dict, temp_ini_str,
                            slaves_list_line_buffer) == -1) {
                        errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                        break;
                    }
                }

                if (net_if_bridge == TRUE) {
                    /* For bridge interfaces, we need to check if any member
                     * interfaces were selected (or removed) */
                    for (i = 0; i < poten_br_member_cnt; i++) {
                        if (br_member_select->selections[i] == 1) {
                            SAFE_ASPRINTF(&temp_pstr, "%s,",
                                    poten_br_members[i]);
                            /* We add one extra for the null byte */
                            br_member_val_size = strlen(temp_pstr) + 1;
                            br_members_line_size = br_members_line_size +
                                    br_member_val_size;
                            if (br_members_line_size >=
                                    MAX_BR_MEMBERS_LIST_BUFF) {
                                errorDialog(main_cdk_screen, "The maximum "
                                        "bridge members list line buffer "
                                        "size has been reached!", NULL);
                                FREE_NULL(temp_pstr);
                                break;
                            } else {
                                strcat(br_members_list_line_buffer, temp_pstr);
                                FREE_NULL(temp_pstr);
                            }
                        }
                    }
                    /* Remove the trailing comma (if any) */
                    if (br_members_list_line_buffer[0] != '\0') {
                        temp_int = strlen(br_members_list_line_buffer) - 1;
                        if (br_members_list_line_buffer[temp_int] == ',')
                            br_members_list_line_buffer[temp_int] = '\0';
                    }
                    /* Set the INI value */
                    snprintf(temp_ini_str, MAX_INI_VAL,
                            "%s:brmembers", net_if_name);
                    if (iniparser_set(ini_dict, temp_ini_str,
                            br_members_list_line_buffer) == -1) {
                        errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                        break;
                    }
                }

                if (ethtool_opts) {
                    /* Store options for ethtool if the widget exists */
                    snprintf(ethtool_opts_buffer, MAX_ETHTOOL_OPTS_BUFF, "%s",
                            getCDKMentryValue(ethtool_opts));
                    snprintf(temp_ini_str, MAX_INI_VAL, "%s:ethtoolopts",
                            net_if_name);
                    if (iniparser_set(ini_dict, temp_ini_str,
                            ethtool_opts_buffer) == -1) {
                        errorDialog(main_cdk_screen,
                                "Couldn't set configuration file value!", NULL);
                        break;
                    }
                }

                if (vlan_egress_map) {
                    /* Store VLAN egress priority map if the widget exists */
                    snprintf(vlan_egress_map_buffer, MAX_VLAN_EGRESS_MAP_BUFF,
                            "%s", getCDKMentryValue(vlan_egress_map));
                    snprintf(temp_ini_str, MAX_INI_VAL, "%s:vlanegressmap",
                            net_if_name);
                    if (iniparser_set(ini_dict, temp_ini_str,
                            vlan_egress_map_buffer) == -1) {
                        errorDialog(main_cdk_screen,
                                "Couldn't set configuration file value!", NULL);
                        break;
                    }
                }

                /* Write to network config. file */
                ini_file = fopen(NETWORK_CONF, "w");
                if (ini_file == NULL) {
                    SAFE_ASPRINTF(&error_msg, NET_CONF_WRITE_ERR,
                            strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                } else {
                    fprintf(ini_file, NET_CONF_COMMENT_1);
                    fprintf(ini_file, INI_CONF_COMMENT);
                    iniparser_dump_ini(ini_dict, ini_file);
                    fclose(ini_file);
                    iniparser_freedict(ini_dict);
                }
            }
        }
        break;
    }

    /* All done -- clean up */
    for (i = 0; i < MAX_NET_IFACE; i++) {
        /* These are allocated in getNetConfChoice() but free'd here */
        FREE_NULL(poten_slaves[i]);
        FREE_NULL(poten_br_members[i]);
    }
    if (net_screen != NULL) {
        destroyCDKScreenObjects(net_screen);
        destroyCDKScreen(net_screen);
    }
    for (i = 0; i < MAX_NET_INFO_LINES; i++)
        FREE_NULL(net_info_msg[i]);
    for (i = 0; i < NET_SHORT_INFO_LINES; i++)
        FREE_NULL(short_label_msg[i]);
    delwin(net_window);
    curs_set(0);
    refreshCDKScreen(main_cdk_screen);

    /* Ask user if they want to restart networking */
    if (traverse_ret == 1) {
        question = questionDialog(main_cdk_screen,
                "Would you like to restart networking now?", NULL);
        if (question)
            restartNetDialog(main_cdk_screen);
    }
    return;
}


/**
 * @brief Run the "Restart Networking" dialog.
 */
void restartNetDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *net_restart_info = 0;
    char *swindow_info[MAX_NET_RESTART_INFO_LINES] = {NULL};
    char *error_msg = NULL, *swindow_title = NULL;
    char net_rc_cmd[MAX_SHELL_CMD_LEN] = {0}, line[NET_RESTART_INFO_COLS] = {0};
    int i = 0, status = 0;
    FILE *net_rc = NULL;
    boolean confirm = FALSE;

    /* Get confirmation (and warn user) before restarting network */
    confirm = confirmDialog(main_cdk_screen,
            "If you are connected via SSH, you may lose your session!",
            "Are you sure you want to restart networking?");
    if (!confirm)
        return;

    /* Setup scrolling window widget */
    SAFE_ASPRINTF(&swindow_title,
            "<C></%d/B>Restarting networking services...\n",
            g_color_dialog_title[g_curr_theme]);
    net_restart_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            (NET_RESTART_INFO_ROWS + 2), (NET_RESTART_INFO_COLS + 2),
            swindow_title, MAX_NET_RESTART_INFO_LINES, TRUE, FALSE);
    if (!net_restart_info) {
        errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(net_restart_info,
            g_color_dialog_text[g_curr_theme]);
    setCDKSwindowBoxAttribute(net_restart_info,
            g_color_dialog_box[g_curr_theme]);
    drawCDKSwindow(net_restart_info, TRUE);

    /* Set our line counter */
    i = 0;

    while (1) {
        /* Stop networking */
        if (i < MAX_NET_RESTART_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[i], "</B>Stopping network:<!B>");
            addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
            i++;
        }
        snprintf(net_rc_cmd, MAX_SHELL_CMD_LEN, "%s stop", RC_NETWORK);
        net_rc = popen(net_rc_cmd, "r");
        if (!net_rc) {
            SAFE_ASPRINTF(&error_msg, "popen(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        } else {
            while (fgets(line, sizeof (line), net_rc) != NULL) {
                if (i < MAX_NET_RESTART_INFO_LINES) {
                    SAFE_ASPRINTF(&swindow_info[i], "%s", line);
                    addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
                    i++;
                }
            }
            status = pclose(net_rc);
            if (status == -1) {
                SAFE_ASPRINTF(&error_msg, "pclose(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            } else {
                if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
                    SAFE_ASPRINTF(&error_msg, "The %s command exited with %d.",
                            RC_NETWORK, WEXITSTATUS(status));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }

        /* Start networking */
        if (i < MAX_NET_RESTART_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[i], " ");
            addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
            i++;
        }
        if (i < MAX_NET_RESTART_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[i], "</B>Starting network:<!B>");
            addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
            i++;
        }
        snprintf(net_rc_cmd, MAX_SHELL_CMD_LEN, "%s start", RC_NETWORK);
        net_rc = popen(net_rc_cmd, "r");
        if (!net_rc) {
            SAFE_ASPRINTF(&error_msg, "popen(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        } else {
            while (fgets(line, sizeof (line), net_rc) != NULL) {
                if (i < MAX_NET_RESTART_INFO_LINES) {
                    SAFE_ASPRINTF(&swindow_info[i], "%s", line);
                    addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
                    i++;
                }
            }
            status = pclose(net_rc);
            if (status == -1) {
                SAFE_ASPRINTF(&error_msg, "pclose(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            } else {
                if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
                    SAFE_ASPRINTF(&error_msg, "The %s command exited with %d.",
                            RC_NETWORK, WEXITSTATUS(status));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }
        if (i < MAX_NET_RESTART_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[i], " ");
            addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
            i++;
        }
        if (i < MAX_NET_RESTART_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[i], CONTINUE_MSG);
            addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
            i++;
        }

        /* Activate, but don't scroll to the top */
        activateCDKSwindow(net_restart_info, 0);
        break;
    }

    /* Done */
    if (net_restart_info)
        destroyCDKSwindow(net_restart_info);
    FREE_NULL(swindow_title);
    for (i = 0; i < MAX_NET_RESTART_INFO_LINES; i++)
        FREE_NULL(swindow_info[i]);
    return;
}


/**
 * @brief Run the "Mail Setup" dialog.
 */
void mailDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *mail_window = 0;
    CDKSCREEN *mail_screen = 0;
    CDKLABEL *mail_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *email_addr = 0, *smtp_host = 0, *smtp_port = 0,
            *auth_user = 0, *auth_pass = 0;
    CDKRADIO *use_tls = 0, *use_starttls = 0, *auth_method = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int i = 0, traverse_ret = 0, window_y = 0, window_x = 0,
            mail_window_lines = 0, mail_window_cols = 0;
    char new_mailhub[MAX_INI_VAL] = {0}, new_authmethod[MAX_INI_VAL] = {0},
            new_usetls[MAX_INI_VAL] = {0}, new_usestarttls[MAX_INI_VAL] = {0},
            hostname[MISC_STRING_LEN] = {0};
    char *mailhub_host = NULL, *mailhub_port = NULL, *error_msg = NULL;
    const char *conf_root = NULL, *conf_mailhub = NULL, *conf_authuser = NULL,
            *conf_authpass = NULL, *conf_authmethod = NULL,
            *conf_usetls = NULL, *conf_usestarttls = NULL;
    char *mail_title_msg[1] = {NULL};
    dictionary *ini_dict = NULL;
    FILE *ini_file = NULL;
    boolean question = FALSE;

    /* Read sSMTP configuration file (INI file) */
    ini_dict = iniparser_load(SSMTP_CONF);
    if (ini_dict == NULL) {
        errorDialog(main_cdk_screen, SSMTP_CONF_READ_ERR, NULL);
        return;
    }
    conf_root = iniparser_getstring(ini_dict, ":root", "");
    conf_mailhub = iniparser_getstring(ini_dict, ":mailhub", "");
    conf_authuser = iniparser_getstring(ini_dict, ":authuser", "");
    conf_authpass = iniparser_getstring(ini_dict, ":authpass", "");
    conf_authmethod = iniparser_getstring(ini_dict, ":authmethod", "NOT_SET");
    conf_usetls = iniparser_getstring(ini_dict, ":usetls", "");
    conf_usestarttls = iniparser_getstring(ini_dict, ":usestarttls", "");

    while (1) {
        /* Get the host name here (used below in sSMTP configuration) */
        if (gethostname(hostname, ((sizeof hostname) - 1)) == -1) {
            SAFE_ASPRINTF(&error_msg, "gethostname(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Setup a new CDK screen for mail setup */
        mail_window_lines = 17;
        mail_window_cols = 68;
        window_y = ((LINES / 2) - (mail_window_lines / 2));
        window_x = ((COLS / 2) - (mail_window_cols / 2));
        mail_window = newwin(mail_window_lines, mail_window_cols,
                window_y, window_x);
        if (mail_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        mail_screen = initCDKScreen(mail_window);
        if (mail_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(mail_window, g_color_dialog_box[g_curr_theme]);
        wbkgd(mail_window, g_color_dialog_text[g_curr_theme]);
        wrefresh(mail_window);

        /* Screen title label */
        SAFE_ASPRINTF(&mail_title_msg[0],
                "</%d/B>System mail (SMTP) settings...",
                g_color_dialog_title[g_curr_theme]);
        mail_label = newCDKLabel(mail_screen, (window_x + 1), (window_y + 1),
                mail_title_msg, 1, FALSE, FALSE);
        if (!mail_label) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(mail_label,
                g_color_dialog_text[g_curr_theme]);

        /* Email address (to send alerts to) field */
        email_addr = newCDKEntry(mail_screen, (window_x + 1), (window_y + 3),
                NULL, "</B>Alert Email Address: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                MAX_EMAIL_LEN, 0, MAX_EMAIL_LEN, FALSE, FALSE);
        if (!email_addr) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(email_addr, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(email_addr,
                    g_color_dialog_text[g_curr_theme]);
        setCDKEntryValue(email_addr, conf_root);

        /* Split up mailhub string from configuration */
        mailhub_host = conf_mailhub;
        if ((mailhub_port = strchr(conf_mailhub, ':')) != NULL) {
            *mailhub_port = '\0';
            mailhub_port++;
        }

        /* SMTP host field */
        smtp_host = newCDKEntry(mail_screen, (window_x + 1), (window_y + 5),
                NULL, "</B>SMTP Host: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                24, 0, MAX_SMTP_LEN, FALSE, FALSE);
        if (!smtp_host) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(smtp_host, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(smtp_host,
                    g_color_dialog_text[g_curr_theme]);
        setCDKEntryValue(smtp_host, mailhub_host);

        /* SMTP port field */
        smtp_port = newCDKEntry(mail_screen, (window_x + 38), (window_y + 5),
                NULL, "</B>SMTP Port: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vINT,
                5, 0, 5, FALSE, FALSE);
        if (!smtp_port) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(smtp_port, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(smtp_port,
                    g_color_dialog_text[g_curr_theme]);
        if (mailhub_port != NULL)
            setCDKEntryValue(smtp_port, mailhub_port);

        /* TLS radio */
        use_tls = newCDKRadio(mail_screen, (window_x + 1), (window_y + 7),
                NONE, 5, 10, "</B>Use TLS", g_no_yes_opts, 2,
                '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!use_tls) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(use_tls, g_color_dialog_text[g_curr_theme]);
        if (strcasecmp(conf_usetls, "yes") == 0)
            setCDKRadioCurrentItem(use_tls, 1);
        else
            setCDKRadioCurrentItem(use_tls, 0);

        /* STARTTLS radio */
        use_starttls = newCDKRadio(mail_screen, (window_x + 16), (window_y + 7),
                NONE, 5, 10, "</B>Use STARTTLS", g_no_yes_opts, 2,
                '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!use_starttls) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(use_starttls,
                g_color_dialog_text[g_curr_theme]);
        if (strcasecmp(conf_usestarttls, "yes") == 0)
            setCDKRadioCurrentItem(use_starttls, 1);
        else
            setCDKRadioCurrentItem(use_starttls, 0);

        /* Auth. Method radio */
        auth_method = newCDKRadio(mail_screen, (window_x + 31), (window_y + 7),
                NONE, 5, 10, "</B>Auth. Method", g_auth_meth_opts, 3,
                '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!auth_method) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(auth_method,
                g_color_dialog_text[g_curr_theme]);
        if (strcasecmp(conf_authmethod, "NOT_SET") == 0)
            setCDKRadioCurrentItem(auth_method, 0);
        else if (strcasecmp(conf_authmethod, "cram-md5") == 0)
            setCDKRadioCurrentItem(auth_method, 2);
        else
            setCDKRadioCurrentItem(auth_method, 1);

        /* Auth. User field */
        auth_user = newCDKEntry(mail_screen, (window_x + 1), (window_y + 12),
                NULL, "</B>Auth. User: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                15, 0, MAX_SMTP_USER_LEN, FALSE, FALSE);
        if (!auth_user) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(auth_user, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(auth_user,
                    g_color_dialog_text[g_curr_theme]);
        setCDKEntryValue(auth_user, conf_authuser);

        /* Auth. Password field */
        auth_pass = newCDKEntry(mail_screen, (window_x + 30), (window_y + 12),
                NULL, "</B>Auth. Password: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                15, 0, MAX_SMTP_PASS_LEN, FALSE, FALSE);
        if (!auth_pass) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(auth_pass, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(auth_pass,
                    g_color_dialog_text[g_curr_theme]);
        setCDKEntryValue(auth_pass, conf_authpass);

        /* Buttons */
        ok_button = newCDKButton(mail_screen, (window_x + 26), (window_y + 15),
                g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(mail_screen, (window_x + 36),
                (window_y + 15), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(mail_screen);
        traverse_ret = traverseCDKScreen(mail_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Check email address (field entry) */
            if (!checkInputStr(main_cdk_screen, EMAIL_CHARS,
                    getCDKEntryValue(email_addr))) {
                traverse_ret = 0; /* Skip the prompt */
                break;
            }

            /* Check SMTP host (field entry) */
            if (!checkInputStr(main_cdk_screen, NAME_CHARS,
                    getCDKEntryValue(smtp_host))) {
                traverse_ret = 0; /* Skip the prompt */
                break;
            }

            /* Check auth. user (field entry) */
            if (strlen(getCDKEntryValue(auth_user)) != 0) {
                if (!checkInputStr(main_cdk_screen, EMAIL_CHARS,
                        getCDKEntryValue(auth_user))) {
                    traverse_ret = 0; /* Skip the prompt */
                    break;
                }
            }

            /* Check auth. password (field entry) */
            if (strlen(getCDKEntryValue(auth_pass)) != 0) {
                if (!checkInputStr(main_cdk_screen, ASCII_CHARS,
                        getCDKEntryValue(auth_pass))) {
                    traverse_ret = 0; /* Skip the prompt */
                    break;
                }
            }

            /* Set config. file */
            if (iniparser_set(ini_dict, ":root",
                    getCDKEntryValue(email_addr)) == -1) {
                errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                break;
            }
            snprintf(new_mailhub, MAX_INI_VAL, "%s:%s",
                    getCDKEntryValue(smtp_host), getCDKEntryValue(smtp_port));
            if (iniparser_set(ini_dict, ":mailhub", new_mailhub) == -1) {
                errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                break;
            }
            if (iniparser_set(ini_dict, ":authuser",
                    getCDKEntryValue(auth_user)) == -1) {
                errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                break;
            }
            if (iniparser_set(ini_dict, ":authpass",
                    getCDKEntryValue(auth_pass)) == -1) {
                errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                break;
            }
            if (getCDKRadioSelectedItem(auth_method) == 0) {
                iniparser_unset(ini_dict, ":authmethod");
                iniparser_unset(ini_dict, ":authuser");
                iniparser_unset(ini_dict, ":authpass");
            } else {
                if (getCDKRadioSelectedItem(auth_method) == 2)
                    snprintf(new_authmethod, MAX_INI_VAL, "CRAM-MD5");
                else if (getCDKRadioSelectedItem(auth_method) == 1)
                    snprintf(new_authmethod, MAX_INI_VAL, " ");
                if (iniparser_set(ini_dict, ":authmethod",
                        new_authmethod) == -1) {
                    errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                    break;
                }
            }
            if (getCDKRadioSelectedItem(use_tls) == 1)
                snprintf(new_usetls, MAX_INI_VAL, "YES");
            else
                snprintf(new_usetls, MAX_INI_VAL, "NO");
            if (iniparser_set(ini_dict, ":usetls", new_usetls) == -1) {
                errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                break;
            }
            if (getCDKRadioSelectedItem(use_starttls) == 1)
                snprintf(new_usestarttls, MAX_INI_VAL, "YES");
            else
                snprintf(new_usestarttls, MAX_INI_VAL, "NO");
            if (iniparser_set(ini_dict, ":usestarttls",
                    new_usestarttls) == -1) {
                errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                break;
            }
            if (iniparser_set(ini_dict, ":hostname", hostname) == -1) {
                errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                break;
            }

            /* Write the configuration file */
            ini_file = fopen(SSMTP_CONF, "w");
            if (ini_file == NULL) {
                SAFE_ASPRINTF(&error_msg, SSMTP_CONF_WRITE_ERR,
                        strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
            } else {
                /* sSMTP is very picky about its configuration file,
                 * so we can't use the iniparser_dump_ini function */
                fprintf(ini_file, SSMTP_CONF_COMMENT_1);
                fprintf(ini_file, INI_CONF_COMMENT);
                for (i = 0; i < ini_dict->size; i++) {
                    if (ini_dict->key[i] == NULL)
                        continue;
                    fprintf(ini_file, "%s=%s\n", ini_dict->key[i] + 1,
                            ini_dict->val[i] ? ini_dict->val[i] : "");
                }
                fclose(ini_file);
            }
        }
        break;
    }

    /* All done -- clean up */
    iniparser_freedict(ini_dict);
    FREE_NULL(mail_title_msg[0]);
    if (mail_screen != NULL) {
        destroyCDKScreenObjects(mail_screen);
        destroyCDKScreen(mail_screen);
    }
    delwin(mail_window);
    curs_set(0);
    refreshCDKScreen(main_cdk_screen);

    /* Ask user if they want to send a test email message */
    if (traverse_ret == 1) {
        question = questionDialog(main_cdk_screen,
                "Would you like to send a test email message?", NULL);
        if (question)
            testEmailDialog(main_cdk_screen);
    }
    return;
}


/**
 * @brief Run the "Send Test Email" dialog.
 */
void testEmailDialog(CDKSCREEN *main_cdk_screen) {
    CDKLABEL *test_email_label = 0;
    char ssmtp_cmd[MAX_SHELL_CMD_LEN] = {0}, email_addy[MAX_EMAIL_LEN] = {0};
    char *message[5] = {NULL};
    char *error_msg = NULL;
    const char *conf_root = NULL;
    int i = 0, status = 0;
    dictionary *ini_dict = NULL;
    FILE *ssmtp = NULL;

    /* Get the email address from the config. file */
    ini_dict = iniparser_load(SSMTP_CONF);
    if (ini_dict == NULL) {
        errorDialog(main_cdk_screen, SSMTP_CONF_READ_ERR, NULL);
        return;
    }
    conf_root = iniparser_getstring(ini_dict, ":root", "");
    if (strcmp(conf_root, "") == 0) {
        errorDialog(main_cdk_screen,
                "No email address is set in the configuration file!", NULL);
        return;
    }

    while (1) {
        /* Display a nice short label message while we send the email */
        snprintf(email_addy, MAX_EMAIL_LEN, "%s", conf_root);
        SAFE_ASPRINTF(&message[0], " ");
        SAFE_ASPRINTF(&message[1], " ");
        SAFE_ASPRINTF(&message[2], "</B>   Sending a test email to %s...   ",
                email_addy);
        SAFE_ASPRINTF(&message[3], " ");
        SAFE_ASPRINTF(&message[4], " ");
        test_email_label = newCDKLabel(main_cdk_screen, CENTER, CENTER,
                message, 5, TRUE, FALSE);
        if (!test_email_label) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(test_email_label,
                g_color_dialog_text[g_curr_theme]);
        setCDKLabelBoxAttribute(test_email_label,
                g_color_dialog_box[g_curr_theme]);
        refreshCDKScreen(main_cdk_screen);

        /* Send the test email message */
        snprintf(ssmtp_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1",
                SSMTP_BIN, email_addy);
        ssmtp = popen(ssmtp_cmd, "w");
        if (!ssmtp) {
            SAFE_ASPRINTF(&error_msg, "popen(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        } else {
            fprintf(ssmtp, "To: %s\n", email_addy);
            fprintf(ssmtp, "From: root\n");
            fprintf(ssmtp, "Subject: ESOS Test Email Message\n\n");
            fprintf(ssmtp, "This is an email from ESOS to verify/confirm "
                    "your email settings.");
            status = pclose(ssmtp);
            if (status == -1) {
                SAFE_ASPRINTF(&error_msg, "pclose(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            } else {
                if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
                    SAFE_ASPRINTF(&error_msg, "The %s command exited with %d.",
                            SSMTP_BIN, WEXITSTATUS(status));
                    errorDialog(main_cdk_screen, error_msg,
                            "Check the mail log for more information.");
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }
        break;
    }

    /* Done */
    iniparser_freedict(ini_dict);
    if (test_email_label)
        destroyCDKLabel(test_email_label);
    for (i = 0; i < 5; i++)
        FREE_NULL(message[i]);
    return;
}


/**
 * @brief Run the "Add User" dialog.
 */
void addUserDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *add_user_window = 0;
    CDKSCREEN *add_user_screen = 0;
    CDKLABEL *add_user_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *uname_field = 0, *pass_1_field = 0, *pass_2_field = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int traverse_ret = 0, window_y = 0, window_x = 0, ret_val = 0,
            exit_stat = 0, add_user_window_lines = 0, add_user_window_cols = 0;
    char add_user_cmd[MAX_SHELL_CMD_LEN] = {0},
            chg_pass_cmd[MAX_SHELL_CMD_LEN] = {0},
            password_1[MAX_PASSWD_LEN] = {0}, password_2[MAX_PASSWD_LEN] = {0};
    char *error_msg = NULL;
    char *add_user_title_msg[1] = {NULL};

    while (1) {
        /* Setup a new CDK screen for adding a new user account */
        add_user_window_lines = 12;
        add_user_window_cols = 50;
        window_y = ((LINES / 2) - (add_user_window_lines / 2));
        window_x = ((COLS / 2) - (add_user_window_cols / 2));
        add_user_window = newwin(add_user_window_lines, add_user_window_cols,
                window_y, window_x);
        if (add_user_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        add_user_screen = initCDKScreen(add_user_window);
        if (add_user_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(add_user_window, g_color_dialog_box[g_curr_theme]);
        wbkgd(add_user_window, g_color_dialog_text[g_curr_theme]);
        wrefresh(add_user_window);

        /* Screen title label */
        SAFE_ASPRINTF(&add_user_title_msg[0],
                "</%d/B>Adding a new ESOS user account...",
                g_color_dialog_title[g_curr_theme]);
        add_user_label = newCDKLabel(add_user_screen, (window_x + 1),
                (window_y + 1), add_user_title_msg, 1, FALSE, FALSE);
        if (!add_user_label) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(add_user_label,
                g_color_dialog_text[g_curr_theme]);

        /* Username field */
        uname_field = newCDKEntry(add_user_screen, (window_x + 1),
                (window_y + 3), NULL, "</B>Username: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                MAX_UNAME_LEN, 0, MAX_UNAME_LEN, FALSE, FALSE);
        if (!uname_field) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(uname_field,
                g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(uname_field,
                    g_color_dialog_text[g_curr_theme]);

        /* Password field (1) */
        pass_1_field = newCDKEntry(add_user_screen, (window_x + 1),
                (window_y + 5), NULL, "</B>User Password:   ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vHMIXED,
                25, 0, MAX_PASSWD_LEN, FALSE, FALSE);
        if (!pass_1_field) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(pass_1_field,
                g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(pass_1_field,
                    g_color_dialog_text[g_curr_theme]);
        setCDKEntryHiddenChar(pass_1_field,
                '*' | g_color_dialog_select[g_curr_theme]);

        /* Password field (2) */
        pass_2_field = newCDKEntry(add_user_screen, (window_x + 1),
                (window_y + 6), NULL, "</B>Retype Password: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vHMIXED,
                25, 0, MAX_PASSWD_LEN, FALSE, FALSE);
        if (!pass_2_field) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(pass_2_field,
                g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(pass_2_field,
                    g_color_dialog_text[g_curr_theme]);
        setCDKEntryHiddenChar(pass_2_field,
                '*' | g_color_dialog_select[g_curr_theme]);

        /* Buttons */
        ok_button = newCDKButton(add_user_screen, (window_x + 17),
                (window_y + 10), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(add_user_screen, (window_x + 27),
                (window_y + 10), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(add_user_screen);
        traverse_ret = traverseCDKScreen(add_user_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Check username (field entry) */
            if (!checkInputStr(main_cdk_screen, NAME_CHARS,
                    getCDKEntryValue(uname_field)))
                break;

            /* Make sure the password fields match */
            strncpy(password_1, getCDKEntryValue(pass_1_field), MAX_PASSWD_LEN);
            password_1[sizeof password_1 - 1] = '\0';
            strncpy(password_2, getCDKEntryValue(pass_2_field), MAX_PASSWD_LEN);
            password_2[sizeof password_2 - 1] = '\0';
            if (strcmp(password_1, password_2) != 0) {
                errorDialog(main_cdk_screen,
                        "The given passwords do not match!", NULL);
                break;
            }

            /* Check first password field (we assume both match if
             * we got this far) */
            if (!checkInputStr(main_cdk_screen, ASCII_CHARS, password_1))
                break;

            /* Add the new user account */
            snprintf(add_user_cmd, MAX_SHELL_CMD_LEN, "%s -h %s -g "
                    "'ESOS User' -s %s -G %s -D %s > /dev/null 2>&1",
                    ADDUSER_BIN, TEMP_DIR, SHELL_BIN, ESOS_GROUP,
                    getCDKEntryValue(uname_field));
            ret_val = system(add_user_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, ADDUSER_BIN,
                        exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }

            /* Set the password for the new account */
            snprintf(chg_pass_cmd, MAX_SHELL_CMD_LEN,
                    "echo '%s:%s' | %s -m > /dev/null 2>&1",
                    getCDKEntryValue(uname_field), password_1, CHPASSWD_BIN);
            ret_val = system(chg_pass_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, CHPASSWD_BIN,
                        exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        }
        break;
    }

    /* Done */
    if (add_user_screen != NULL) {
        destroyCDKScreenObjects(add_user_screen);
        destroyCDKScreen(add_user_screen);
    }
    FREE_NULL(add_user_title_msg[0]);
    delwin(add_user_window);
    return;
}


/**
 * @brief Run the "Delete User" dialog.
 */
void delUserDialog(CDKSCREEN *main_cdk_screen) {
    int ret_val = 0, exit_stat = 0;
    char del_user_cmd[MAX_SHELL_CMD_LEN] = {0},
            del_grp_cmd[MAX_SHELL_CMD_LEN] = {0},
            user_acct[MAX_UNAME_LEN] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    uid_t ruid = 0, euid = 0, suid = 0;
    struct passwd *passwd_entry = NULL;
    boolean confirm = FALSE;

    /* Have the user choose a user account */
    getUserAcct(main_cdk_screen, user_acct);
    if (user_acct[0] == '\0')
        return;

    /* Make sure we are not trying to delete the user that is
     * currently logged in */
    getresuid(&ruid, &euid, &suid);
    passwd_entry = getpwuid(suid);
    if (strcmp(passwd_entry->pw_name, user_acct) == 0) {
        errorDialog(main_cdk_screen,
                "Can't delete the user that is currently logged in!", NULL);
        return;
    }

    /* Can't delete the ESOS superuser account */
    if (strcmp(ESOS_SUPERUSER, user_acct) == 0) {
        errorDialog(main_cdk_screen,
                "Can't delete the ESOS superuser account!", NULL);
        return;
    }

    /* Get a final confirmation from user before we delete */
    SAFE_ASPRINTF(&confirm_msg, "Are you sure you want to delete user '%s'?",
            user_acct);
    confirm = confirmDialog(main_cdk_screen, confirm_msg, NULL);
    FREE_NULL(confirm_msg);
    if (confirm) {
        /* Remove the user from the ESOS users group (deluser
         * doesn't seem to do this) */
        snprintf(del_grp_cmd, MAX_SHELL_CMD_LEN, "%s %s %s > /dev/null 2>&1",
                DELGROUP_BIN, user_acct, ESOS_GROUP);
        ret_val = system(del_grp_cmd);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, DELGROUP_BIN, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            return;
        }

        /* Delete the user account */
        snprintf(del_user_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1",
                DELUSER_BIN, user_acct);
        ret_val = system(del_user_cmd);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, DELUSER_BIN, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            return;
        }
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Change Password" dialog.
 */
void chgPasswdDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *chg_pass_window = 0;
    CDKSCREEN *chg_pass_screen = 0;
    CDKLABEL *passwd_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *new_pass_1 = 0, *new_pass_2 = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int i = 0, traverse_ret = 0, window_y = 0, window_x = 0, ret_val = 0,
            exit_stat = 0, chg_pass_window_lines = 0, chg_pass_window_cols = 0;
    char *screen_title[CHG_PASSWD_INFO_LINES] = {NULL};
    char chg_pass_cmd[MAX_SHELL_CMD_LEN] = {0},
            password_1[MAX_PASSWD_LEN] = {0},
            password_2[MAX_PASSWD_LEN] = {0}, user_acct[MAX_UNAME_LEN] = {0};
    char *error_msg = NULL;

    /* Have the user choose a user account */
    getUserAcct(main_cdk_screen, user_acct);
    if (user_acct[0] == '\0')
        return;

    while (1) {
        /* Setup a new CDK screen for password change */
        chg_pass_window_lines = 10;
        chg_pass_window_cols = 45;
        window_y = ((LINES / 2) - (chg_pass_window_lines / 2));
        window_x = ((COLS / 2) - (chg_pass_window_cols / 2));
        chg_pass_window = newwin(chg_pass_window_lines, chg_pass_window_cols,
                window_y, window_x);
        if (chg_pass_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        chg_pass_screen = initCDKScreen(chg_pass_window);
        if (chg_pass_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(chg_pass_window, g_color_dialog_box[g_curr_theme]);
        wbkgd(chg_pass_window, g_color_dialog_text[g_curr_theme]);
        wrefresh(chg_pass_window);

        /* Screen title label */
        SAFE_ASPRINTF(&screen_title[0],
                "</%d/B>Changing password for user %s...",
                g_color_dialog_title[g_curr_theme], user_acct);
        passwd_label = newCDKLabel(chg_pass_screen,
                (window_x + 1), (window_y + 1),
                screen_title, CHG_PASSWD_INFO_LINES, FALSE, FALSE);
        if (!passwd_label) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(passwd_label,
                g_color_dialog_text[g_curr_theme]);

        /* New password field (1) */
        new_pass_1 = newCDKEntry(chg_pass_screen, (window_x + 1),
                (window_y + 3), NULL, "</B>New Password:    ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vHMIXED,
                20, 0, MAX_PASSWD_LEN, FALSE, FALSE);
        if (!new_pass_1) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(new_pass_1,
                g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(new_pass_1,
                    g_color_dialog_text[g_curr_theme]);
        setCDKEntryHiddenChar(new_pass_1,
                '*' | g_color_dialog_select[g_curr_theme]);

        /* New password field (2) */
        new_pass_2 = newCDKEntry(chg_pass_screen, (window_x + 1),
                (window_y + 4), NULL, "</B>Retype Password: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vHMIXED,
                20, 0, MAX_PASSWD_LEN, FALSE, FALSE);
        if (!new_pass_2) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(new_pass_2,
                g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(new_pass_2,
                    g_color_dialog_text[g_curr_theme]);
        setCDKEntryHiddenChar(new_pass_2,
                '*' | g_color_dialog_select[g_curr_theme]);

        /* Buttons */
        ok_button = newCDKButton(chg_pass_screen, (window_x + 14),
                (window_y + 8), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(chg_pass_screen, (window_x + 24),
                (window_y + 8), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(chg_pass_screen);
        traverse_ret = traverseCDKScreen(chg_pass_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Make sure the password fields match */
            strncpy(password_1, getCDKEntryValue(new_pass_1), MAX_PASSWD_LEN);
            password_1[sizeof password_1 - 1] = '\0';
            strncpy(password_2, getCDKEntryValue(new_pass_2), MAX_PASSWD_LEN);
            password_2[sizeof password_2 - 1] = '\0';
            if (strcmp(password_1, password_2) != 0) {
                errorDialog(main_cdk_screen,
                        "The given passwords do not match!", NULL);
                break;
            }

            /* Check first password field (we assume both match
             * if we got this far) */
            if (!checkInputStr(main_cdk_screen, ASCII_CHARS, password_1))
                break;

            /* Set the new password */
            snprintf(chg_pass_cmd, MAX_SHELL_CMD_LEN,
                    "echo '%s:%s' | %s -m > /dev/null 2>&1",
                    user_acct, password_1, CHPASSWD_BIN);
            ret_val = system(chg_pass_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, CHPASSWD_BIN,
                        exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        }
        break;
    }

    /* Done */
    for (i = 0; i < CHG_PASSWD_INFO_LINES; i++)
        FREE_NULL(screen_title[i]);
    if (chg_pass_screen != NULL) {
        destroyCDKScreenObjects(chg_pass_screen);
        destroyCDKScreen(chg_pass_screen);
    }
    delwin(chg_pass_window);
    return;
}


/**
 * @brief Run the "SCST Information" dialog.
 */
void scstInfoDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *scst_info = 0;
    char scst_ver[MAX_SYSFS_ATTR_SIZE] = {0},
            scst_setup_id[MAX_SYSFS_ATTR_SIZE] = {0},
            scst_threads[MAX_SYSFS_ATTR_SIZE] = {0},
            scst_sysfs_res[MAX_SYSFS_ATTR_SIZE] = {0},
            tmp_sysfs_path[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_attr_line[SCST_INFO_COLS] = {0};
    char *swindow_info[MAX_SCST_INFO_LINES] = {NULL};
    char *temp_pstr = NULL, *swindow_title = NULL;
    FILE *sysfs_file = NULL;
    int i = 0;

    /* Setup scrolling window widget */
    SAFE_ASPRINTF(&swindow_title, "<C></%d/B>SCST Information / Statistics\n",
            g_color_dialog_title[g_curr_theme]);
    scst_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            (SCST_INFO_ROWS + 2), (SCST_INFO_COLS + 2),
            swindow_title, MAX_SCST_INFO_LINES, TRUE, FALSE);
    if (!scst_info) {
        errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(scst_info, g_color_dialog_text[g_curr_theme]);
    setCDKSwindowBoxAttribute(scst_info, g_color_dialog_box[g_curr_theme]);

    /* Grab some semi-useful information for our scrolling window widget */
    snprintf(tmp_sysfs_path, MAX_SYSFS_PATH_SIZE,
            "%s/version",SYSFS_SCST_TGT);
    readAttribute(tmp_sysfs_path, scst_ver);
    snprintf(tmp_sysfs_path, MAX_SYSFS_PATH_SIZE,
            "%s/setup_id", SYSFS_SCST_TGT);
    readAttribute(tmp_sysfs_path, scst_setup_id);
    snprintf(tmp_sysfs_path, MAX_SYSFS_PATH_SIZE,
            "%s/threads", SYSFS_SCST_TGT);
    readAttribute(tmp_sysfs_path, scst_threads);
    snprintf(tmp_sysfs_path, MAX_SYSFS_PATH_SIZE,
            "%s/last_sysfs_mgmt_res", SYSFS_SCST_TGT);
    readAttribute(tmp_sysfs_path, scst_sysfs_res);

    /* Add the attribute values collected above to our
     * scrolling window widget */
    SAFE_ASPRINTF(&swindow_info[0],
            "</B>Version:<!B>\t%-15s</B>Setup ID:<!B>\t\t%s",
            scst_ver, scst_setup_id);
    addCDKSwindow(scst_info, swindow_info[0], BOTTOM);
    SAFE_ASPRINTF(&swindow_info[1],
            "</B>Threads:<!B>\t%-15s</B>Last sysfs result:<!B>\t%s",
            scst_threads, scst_sysfs_res);
    addCDKSwindow(scst_info, swindow_info[1], BOTTOM);

     /* Loop over the SGV global statistics attribute/file in sysfs */
    SAFE_ASPRINTF(&swindow_info[2], " ");
    addCDKSwindow(scst_info, swindow_info[2], BOTTOM);
    SAFE_ASPRINTF(&swindow_info[3], "</B>Global SGV cache statistics:<!B>");
    addCDKSwindow(scst_info, swindow_info[3], BOTTOM);
    i = 4;
    snprintf(tmp_sysfs_path, MAX_SYSFS_PATH_SIZE,
            "%s/sgv/global_stats", SYSFS_SCST_TGT);
    if ((sysfs_file = fopen(tmp_sysfs_path, "r")) == NULL) {
        if (i < MAX_SCST_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[i], "fopen(): %s", strerror(errno));
            addCDKSwindow(scst_info, swindow_info[i], BOTTOM);
        }
    } else {
        while (fgets(tmp_attr_line, sizeof (tmp_attr_line),
                sysfs_file) != NULL) {
            if (i < MAX_SCST_INFO_LINES) {
                temp_pstr = strrchr(tmp_attr_line, '\n');
                if (temp_pstr)
                    *temp_pstr = '\0';
                SAFE_ASPRINTF(&swindow_info[i], "%s", tmp_attr_line);
                addCDKSwindow(scst_info, swindow_info[i], BOTTOM);
                i++;
            }
        }
        fclose(sysfs_file);
    }
    if (i < MAX_SCST_INFO_LINES) {
        SAFE_ASPRINTF(&swindow_info[i], " ");
        addCDKSwindow(scst_info, swindow_info[i], BOTTOM);
        i++;
    }
    if (i < MAX_SCST_INFO_LINES) {
        SAFE_ASPRINTF(&swindow_info[i], CONTINUE_MSG);
        addCDKSwindow(scst_info, swindow_info[i], BOTTOM);
        i++;
    }

    /* The 'g' makes the swindow widget scroll to the top, then activate */
    injectCDKSwindow(scst_info, 'g');
    activateCDKSwindow(scst_info, 0);

    /* We fell through -- the user exited the widget, but we don't care how */
    destroyCDKSwindow(scst_info);
    FREE_NULL(swindow_title);
    for (i = 0; i < MAX_SCST_INFO_LINES; i++) {
        FREE_NULL(swindow_info[i]);
    }
    return;
}


/**
 * @brief Run the "CRM Status" dialog.
 */
void crmStatusDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *crm_info = 0;
    char *swindow_info[MAX_CRM_INFO_LINES] = {NULL};
    char *error_msg = NULL, *crm_cmd = NULL, *swindow_title = NULL;
    int i = 0, line_pos = 0, status = 0, ret_val = 0;
    char line[CRM_INFO_COLS] = {0};
    FILE *crm_proc = NULL;

    /* Run the crm command */
    SAFE_ASPRINTF(&crm_cmd, "%s status 2>&1", CRM_TOOL);
    if ((crm_proc = popen(crm_cmd, "r")) == NULL) {
        SAFE_ASPRINTF(&error_msg,
                "Couldn't open process for the %s command!", CRM_TOOL);
        errorDialog(main_cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
    } else {
        /* Add the contents to the scrolling window widget */
        line_pos = 0;
        while (fgets(line, sizeof (line), crm_proc) != NULL) {
            if (line_pos < MAX_CRM_INFO_LINES) {
                SAFE_ASPRINTF(&swindow_info[line_pos], "%s", line);
                line_pos++;
            }
        }

        /* Add a message to the bottom explaining how to close the dialog */
        if (line_pos < MAX_CRM_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[line_pos], " ");
            line_pos++;
        }
        if (line_pos < MAX_CRM_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[line_pos], CONTINUE_MSG);
            line_pos++;
        }

        /* Close the process stream and check exit status */
        if ((status = pclose(crm_proc)) == -1) {
            ret_val = -1;
        } else {
            if (WIFEXITED(status))
                ret_val = WEXITSTATUS(status);
            else
                ret_val = -1;
        }
        if (ret_val == 0) {
            /* Setup scrolling window widget */
            SAFE_ASPRINTF(&swindow_title, "<C></%d/B>CRM Status\n",
                    g_color_dialog_title[g_curr_theme]);
            crm_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
                    (CRM_INFO_ROWS + 2), (CRM_INFO_COLS + 2),
                    swindow_title, MAX_CRM_INFO_LINES, TRUE, FALSE);
            if (!crm_info) {
                errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
                return;
            }
            setCDKSwindowBackgroundAttrib(crm_info,
                    g_color_dialog_text[g_curr_theme]);
            setCDKSwindowBoxAttribute(crm_info,
                    g_color_dialog_box[g_curr_theme]);

            /* Set the scrolling window content */
            setCDKSwindowContents(crm_info, swindow_info, line_pos);

            /* The 'g' makes the swindow widget scroll to the top,
             * then activate */
            injectCDKSwindow(crm_info, 'g');
            activateCDKSwindow(crm_info, 0);

            /* We fell through -- the user exited the widget,
             * but we don't care how */
            destroyCDKSwindow(crm_info);
        } else {
            SAFE_ASPRINTF(&error_msg, "The %s command exited with %d.",
                    CRM_TOOL, ret_val);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    FREE_NULL(swindow_title);
    for (i = 0; i < MAX_CRM_INFO_LINES; i++ )
        FREE_NULL(swindow_info[i]);
    return;
}


/**
 * @brief Run the "Date & Time Settings" dialog.
 */
void dateTimeDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *date_window = 0;
    CDKSCREEN *date_screen = 0;
    CDKLABEL *date_title_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *ntp_server = 0;
    CDKRADIO *tz_select = 0;
    CDKCALENDAR *calendar = 0;
    CDKUSCALE *hour = 0, *minute = 0, *second = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int i = 0, window_y = 0, window_x = 0, traverse_ret = 0, file_cnt = 0,
            curr_tz_item = 0, temp_int = 0, new_day = 0, new_month = 0,
            new_year = 0, new_hour = 0, new_minute = 0, new_second = 0,
            curr_day = 0, curr_month = 0, curr_year = 0, curr_hour = 0,
            curr_minute = 0, curr_second = 0, date_window_lines = 0,
            date_window_cols = 0;
    char *tz_files[MAX_TZ_FILES] = {NULL}, *date_title_msg[1] = {NULL};
    char *error_msg = NULL, *remove_me = NULL, *strstr_result = NULL;
    char zoneinfo_path[MAX_ZONEINFO_PATH] = {0},
            ntp_serv_val[MAX_NTP_LEN] = {0},
            new_ntp_serv_val[MAX_NTP_LEN] = {0},
            dir_name[MAX_ZONEINFO_PATH] = {0};
    FILE *ntp_server_file = NULL;
    DIR *tz_base_dir = NULL, *tz_sub_dir1 = NULL, *tz_sub_dir2 = NULL;
    struct dirent *base_dir_entry = NULL, *sub_dir1_entry = NULL,
            *sub_dir2_entry = NULL;
    struct tm *curr_date_info = NULL;
    time_t curr_clock = 0;
    boolean time_changed = FALSE, finished = FALSE;

    while (1) {
        /* New CDK screen for date and time settings */
        date_window_lines = 20;
        date_window_cols = 66;
        window_y = ((LINES / 2) - (date_window_lines / 2));
        window_x = ((COLS / 2) - (date_window_cols / 2));
        date_window = newwin(date_window_lines, date_window_cols,
                window_y, window_x);
        if (date_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        date_screen = initCDKScreen(date_window);
        if (date_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(date_window, g_color_dialog_box[g_curr_theme]);
        wbkgd(date_window, g_color_dialog_text[g_curr_theme]);
        wrefresh(date_window);

        /* Date/time title label */
        SAFE_ASPRINTF(&date_title_msg[0],
                "</%d/B>Edit date and time settings...",
                g_color_dialog_title[g_curr_theme]);
        date_title_label = newCDKLabel(date_screen, (window_x + 1),
                (window_y + 1), date_title_msg, 1, FALSE, FALSE);
        if (!date_title_label) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(date_title_label,
                g_color_dialog_text[g_curr_theme]);

        /* Get time zone information; we only traverse
         * two directories deep */
        file_cnt = 0;
        if ((tz_base_dir = opendir(ZONEINFO)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }
        while (((base_dir_entry = readdir(tz_base_dir)) != NULL) &&
                (file_cnt < MAX_TZ_FILES)) {
            /* We want to skip the '.' and '..' directories */
            if ((base_dir_entry->d_type == DT_DIR) &&
                    (strcmp(base_dir_entry->d_name, ".") != 0) &&
                    (strcmp(base_dir_entry->d_name, "..") != 0)) {
                snprintf(dir_name, MAX_ZONEINFO_PATH, "%s/%s", ZONEINFO,
                        base_dir_entry->d_name);
                if ((tz_sub_dir1 = opendir(dir_name)) == NULL) {
                    SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    finished = TRUE;
                    break;
                }
                while (((sub_dir1_entry = readdir(tz_sub_dir1)) != NULL) &&
                        (file_cnt < MAX_TZ_FILES)) {
                    /* We want to skip the '.' and '..' directories */
                    if ((sub_dir1_entry->d_type == DT_DIR) &&
                            (strcmp(sub_dir1_entry->d_name, ".") != 0) &&
                            (strcmp(sub_dir1_entry->d_name, "..") != 0)) {
                        snprintf(dir_name, MAX_ZONEINFO_PATH,
                                "%s/%s/%s", ZONEINFO,
                                base_dir_entry->d_name, sub_dir1_entry->d_name);
                        if ((tz_sub_dir2 = opendir(dir_name)) == NULL) {
                            SAFE_ASPRINTF(&error_msg, "opendir(): %s",
                                    strerror(errno));
                            errorDialog(main_cdk_screen, error_msg, NULL);
                            FREE_NULL(error_msg);
                            finished = TRUE;
                            break;
                        }
                        while ((sub_dir2_entry = readdir(tz_sub_dir2)) !=
                                NULL) {
                            if (sub_dir2_entry->d_type == DT_REG) {
                                SAFE_ASPRINTF(&tz_files[file_cnt], "%s/%s/%s",
                                        base_dir_entry->d_name,
                                        sub_dir1_entry->d_name,
                                        sub_dir2_entry->d_name);
                                file_cnt++;
                            }
                        }
                        closedir(tz_sub_dir2);
                    } else if (sub_dir1_entry->d_type == DT_REG) {
                        SAFE_ASPRINTF(&tz_files[file_cnt], "%s/%s",
                                base_dir_entry->d_name, sub_dir1_entry->d_name);
                        file_cnt++;
                    }
                }
                closedir(tz_sub_dir1);
            } else if (base_dir_entry->d_type == DT_REG) {
                SAFE_ASPRINTF(&tz_files[file_cnt], "%s",
                        base_dir_entry->d_name);
                file_cnt++;
            }
        }
        closedir(tz_base_dir);
        if (finished)
            break;

        /* A radio widget for displaying/choosing time zone */
        tz_select = newCDKRadio(date_screen, (window_x + 1), (window_y + 3),
                NONE, 12, 34, "</B>Time Zone\n", tz_files, file_cnt,
                '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!tz_select) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(tz_select,
                g_color_dialog_text[g_curr_theme]);

        /* Get the current time zone data file path (from sym. link) */
        if (readlink(LOCALTIME, zoneinfo_path, MAX_ZONEINFO_PATH) == -1) {
            SAFE_ASPRINTF(&error_msg, "readlink(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Parse the current time zone link target
         * path and set the radio item */
        strstr_result = strstr(zoneinfo_path, ZONEINFO);
        if (strstr_result) {
            strstr_result = strstr_result + (sizeof (ZONEINFO) - 1);
            if (*strstr_result == '/')
                strstr_result++;
            for (i = 0; i < file_cnt; i++) {
                if (strcmp(tz_files[i], strstr_result) == 0) {
                    setCDKRadioCurrentItem(tz_select, i);
                    curr_tz_item = i;
                    break;
                }
            }
        } else {
            setCDKRadioCurrentItem(tz_select, 0);
        }

        /* NTP server */
        ntp_server = newCDKEntry(date_screen, (window_x + 1), (window_y + 16),
                NULL, "</B>NTP Server: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vMIXED, 20,
                0, MAX_NTP_LEN, FALSE, FALSE);
        if (!ntp_server) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(ntp_server, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(ntp_server,
                    g_color_dialog_text[g_curr_theme]);

        /* Get the current NTP server setting (if any) and set widget */
        if ((ntp_server_file = fopen(NTP_SERVER, "r")) == NULL) {
            /* ENOENT is okay since its possible this file doesn't exist yet */
            if (errno != ENOENT) {
                SAFE_ASPRINTF(&error_msg, "fopen(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        } else {
            fgets(ntp_serv_val, MAX_NTP_LEN, ntp_server_file);
            fclose(ntp_server_file);
            remove_me = strrchr(ntp_serv_val, '\n');
            if (remove_me)
                *remove_me = '\0';
            setCDKEntryValue(ntp_server, ntp_serv_val);
        }

        /* Get current date/time information */
        time(&curr_clock);
        curr_date_info = localtime(&curr_clock);
        curr_day = curr_date_info->tm_mday;
        curr_month = curr_date_info->tm_mon + 1;
        curr_year = curr_date_info->tm_year + 1900;
        curr_hour = curr_date_info->tm_hour;
        curr_minute = curr_date_info->tm_min;
        curr_second = curr_date_info->tm_sec;

        /* Calendar widget for displaying/setting current date */
        calendar = newCDKCalendar(date_screen, (window_x + 40), (window_y + 3),
                "</B>Current Date", curr_day, curr_month, curr_year,
                g_color_dialog_text[g_curr_theme],
                g_color_dialog_text[g_curr_theme],
                g_color_dialog_text[g_curr_theme],
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!calendar) {
            errorDialog(main_cdk_screen, CALENDAR_ERR_MSG, NULL);
            break;
        }
        setCDKCalendarBackgroundAttrib(calendar,
                g_color_dialog_text[g_curr_theme]);

        /* Hour, minute, second scale widgets */
        hour = newCDKUScale(date_screen, (window_x + 39), (window_y + 15),
                "</B>Hour  ", NULL, g_color_dialog_input[g_curr_theme], 3,
                curr_hour, 0, 23, 1, 5, FALSE, FALSE);
        if (!hour) {
            errorDialog(main_cdk_screen, SCALE_ERR_MSG, NULL);
            break;
        }
        setCDKUScaleBackgroundAttrib(hour, g_color_dialog_text[g_curr_theme]);
        minute = newCDKUScale(date_screen, (window_x + 47), (window_y + 15),
                "</B>Minute", NULL, g_color_dialog_input[g_curr_theme], 3,
                curr_minute, 0, 59,
                1, 5, FALSE, FALSE);
        if (!minute) {
            errorDialog(main_cdk_screen, SCALE_ERR_MSG, NULL);
            break;
        }
        setCDKUScaleBackgroundAttrib(minute, g_color_dialog_text[g_curr_theme]);
        second = newCDKUScale(date_screen, (window_x + 55), (window_y + 15),
                "</B>Second", NULL, g_color_dialog_input[g_curr_theme], 3,
                curr_second, 0, 59,
                1, 5, FALSE, FALSE);
        if (!second) {
            errorDialog(main_cdk_screen, SCALE_ERR_MSG, NULL);
            break;
        }
        setCDKUScaleBackgroundAttrib(second, g_color_dialog_text[g_curr_theme]);

        /* Buttons */
        ok_button = newCDKButton(date_screen, (window_x + 24), (window_y + 18),
                g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(date_screen, (window_x + 34),
                (window_y + 18), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(date_screen);
        traverse_ret = traverseCDKScreen(date_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Check time zone radio */
            temp_int = getCDKRadioSelectedItem(tz_select);
            /* If the time zone setting was changed, create a new sym. link */
            if (temp_int != curr_tz_item) {
                if (unlink(LOCALTIME) == -1) {
                    SAFE_ASPRINTF(&error_msg, "unlink(): %s", strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                } else {
                    snprintf(dir_name, MAX_ZONEINFO_PATH, "%s/%s",
                            ZONEINFO, tz_files[temp_int]);
                    if (symlink(dir_name, LOCALTIME) == -1) {
                        SAFE_ASPRINTF(&error_msg, "symlink(): %s",
                                strerror(errno));
                        errorDialog(main_cdk_screen, error_msg, NULL);
                        FREE_NULL(error_msg);
                        break;
                    }
                }
            }

            /* Check NTP server setting (field entry) */
            strncpy(new_ntp_serv_val, getCDKEntryValue(ntp_server),
                    MAX_NTP_LEN);
            new_ntp_serv_val[sizeof new_ntp_serv_val - 1] = '\0';
            if (strlen(new_ntp_serv_val) != 0) {
                if (!checkInputStr(main_cdk_screen,
                        NAME_CHARS, new_ntp_serv_val))
                    break;
            }

            /* If the value has changed, write it to the file */
            if (strcmp(ntp_serv_val, new_ntp_serv_val) != 0) {
                if ((ntp_server_file = fopen(NTP_SERVER, "w+")) == NULL) {
                    SAFE_ASPRINTF(&error_msg, "fopen(): %s", strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                } else {
                    fprintf(ntp_server_file, "%s", new_ntp_serv_val);
                    if (fclose(ntp_server_file) != 0) {
                        SAFE_ASPRINTF(&error_msg, "fclose(): %s",
                                strerror(errno));
                        errorDialog(main_cdk_screen, error_msg, NULL);
                        FREE_NULL(error_msg);
                        break;
                    }
                }
            }

            /* Get/check date/time settings */
            getCDKCalendarDate(calendar, &new_day, &new_month, &new_year);
            new_hour = getCDKUScaleValue(hour);
            new_minute = getCDKUScaleValue(minute);
            new_second = getCDKUScaleValue(second);
            if (new_day != curr_day) {
                curr_date_info->tm_mday = new_day;
                time_changed = TRUE;
            }
            if (new_month != curr_month) {
                curr_date_info->tm_mon = new_month - 1;
                time_changed = TRUE;
            }
            if (new_year != curr_year) {
                curr_date_info->tm_year = new_year - 1900;
                time_changed = TRUE;
            }
            if (new_hour != curr_hour) {
                curr_date_info->tm_hour = new_hour;
                time_changed = TRUE;
            }
            if (new_minute != curr_minute) {
                curr_date_info->tm_min = new_minute;
                time_changed = TRUE;
            }
            if (new_second != curr_second) {
                curr_date_info->tm_sec = new_second;
                time_changed = TRUE;
            }
            /* Set date & time (if it changed) */
            if (time_changed) {
                curr_date_info->tm_isdst = -1;
                const struct timeval time_val = {mktime(curr_date_info), 0};
                if (settimeofday(&time_val, 0) == -1) {
                    SAFE_ASPRINTF(&error_msg, "settimeofday(): %s",
                            strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }
        break;
    }

    /* All done -- clean up */
    if (date_screen != NULL) {
        destroyCDKScreenObjects(date_screen);
        destroyCDKScreen(date_screen);
    }
    FREE_NULL(date_title_msg[0]);
    delwin(date_window);
    for (i = 0; i < MAX_TZ_FILES; i++)
        FREE_NULL(tz_files[i]);
    return;
}


/**
 * @brief Run the "DRBD Status" dialog.
 */
void drbdStatDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *drbd_info = 0;
    char *swindow_info[MAX_DRBD_INFO_LINES] = {NULL};
    char *error_msg = NULL, *swindow_title = NULL;
    int i = 0, line_pos = 0;
    char line[DRBD_INFO_COLS] = {0};
    FILE *drbd_file = NULL;

    /* Open the file */
    if ((drbd_file = fopen(PROC_DRBD, "r")) == NULL) {
        SAFE_ASPRINTF(&error_msg, "fopen(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
    } else {
        /* Setup scrolling window widget */
        SAFE_ASPRINTF(&swindow_title, "<C></%d/B>Distributed Replicated Block "
                "Device (DRBD) Information\n",
                g_color_dialog_title[g_curr_theme]);
        drbd_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
                (DRBD_INFO_ROWS + 2), (DRBD_INFO_COLS + 2),
                swindow_title, MAX_DRBD_INFO_LINES, TRUE, FALSE);
        if (!drbd_info) {
            errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
            fclose(drbd_file);
            return;
        }
        setCDKSwindowBackgroundAttrib(drbd_info,
                g_color_dialog_text[g_curr_theme]);
        setCDKSwindowBoxAttribute(drbd_info, g_color_dialog_box[g_curr_theme]);

        /* Add the contents to the scrolling window widget */
        line_pos = 0;
        while (fgets(line, sizeof (line), drbd_file) != NULL) {
            if (line_pos < MAX_DRBD_INFO_LINES) {
                SAFE_ASPRINTF(&swindow_info[line_pos], "%s", line);
                line_pos++;
            }
        }
        fclose(drbd_file);

        /* Add a message to the bottom explaining how to close the dialog */
        if (line_pos < MAX_DRBD_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[line_pos], " ");
            line_pos++;
        }
        if (line_pos < MAX_DRBD_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[line_pos], CONTINUE_MSG);
            line_pos++;
        }

        /* Set the scrolling window content */
        setCDKSwindowContents(drbd_info, swindow_info, line_pos);

        /* The 'g' makes the swindow widget scroll to the top, then activate */
        injectCDKSwindow(drbd_info, 'g');
        activateCDKSwindow(drbd_info, 0);

        /* We fell through -- the user exited the widget, but
         * we don't care how */
        destroyCDKSwindow(drbd_info);
    }

    /* Done */
    FREE_NULL(swindow_title);
    for (i = 0; i < MAX_DRBD_INFO_LINES; i++ )
        FREE_NULL(swindow_info[i]);
    return;
}
