

function update_last_ver_view(mode)
{
    if(mode == 0)
        $('#last_actual_version_text').hide();
    else if(mode == 1)
    {
        $('#last_actual_version_text').show();
        $('#last_actual_version_text').removeClass();
        $('#last_actual_version_text').addClass("daemon_view_param_value_last_ver_info_new");
    }else if(mode == 2)
    {
        $('#last_actual_version_text').show();
        $('#last_actual_version_text').removeClass();
        $('#last_actual_version_text').addClass("daemon_view_param_value_last_ver_info_calm");
    }else if(mode == 3)
    {
        $('#last_actual_version_text').show();
        $('#last_actual_version_text').removeClass();
        $('#last_actual_version_text').addClass("daemon_view_param_value_last_ver_info_urgent");
    }else if(mode == 4)
    {
        $('#last_actual_version_text').show();
        $('#last_actual_version_text').removeClass();
        $('#last_actual_version_text').addClass("daemon_view_param_value_last_ver_info_critical");
    }

}

var aliases_set = {};
function update_aliases_autocompletion()
{
    if(aliases_set.aliases)
        return;

    aliases_set = jQuery.parseJSON(Qt_parent.request_aliases());
    if(aliases_set.aliases)
    {
        console.log("aliases loaded: " + aliases_set.aliases.length);
        var availableTags = [];
        for(var i=0; i < aliases_set.aliases.length; i++)
        {
            availableTags.push("@" + aliases_set.aliases[i].alias);
        }
        $( "#transfer_address_id" ).autocomplete({
            source: availableTags,
            minLength: 2
        });
    }
    else
    {
        console.log("internal error: aliases  not loaded");
    }
}

function on_update_daemon_state(info_obj)
{
    //var info_obj = jQuery.parseJSON(daemon_info_str);

    if(info_obj.daemon_network_state == 0)//daemon_network_state_connecting
    {
        //do nothing
    }else if(info_obj.daemon_network_state == 1)//daemon_network_state_synchronizing
    {
        var percents = 0;
        if (info_obj.max_net_seen_height > info_obj.synchronization_start_height)
            percents = ((info_obj.height - info_obj.synchronization_start_height)*100)/(info_obj.max_net_seen_height - info_obj.synchronization_start_height);
        $("#synchronization_progressbar").progressbar( "option", "value", percents );
        if(info_obj.max_net_seen_height)
            $("#daemon_synchronization_text").text((info_obj.max_net_seen_height - info_obj.height).toString() + " blocks behind");
        $("#open_wallet_button").button("disable");
        $("#generate_wallet_button").button("disable");
        $("#domining_button").button("disable");
        //show progress
    }else if(info_obj.daemon_network_state == 2)//daemon_network_state_online
    {
        $("#synchronization_progressbar_block").hide();
        $("#daemon_status_text").addClass("daemon_view_general_status_value_success_text");
        $("#open_wallet_button").button("enable");
        $("#generate_wallet_button").button("enable");
        disable_tab(document.getElementById('wallet_view_menu'), false);
        //load aliases
        update_aliases_autocompletion();
        //$("#domining_button").button("enable");
        //show OK
    }else if(info_obj.daemon_network_state == 3)//deinit
    {
        $("#synchronization_progressbar_block").show();
        $("#synchronization_progressbar" ).progressbar({value: false });
        $("#daemon_synchronization_text").text("");

        $("#daemon_status_text").removeClass("daemon_view_general_status_value_success_text");
        $("#open_wallet_button").button("disable");
        $("#generate_wallet_button").button("disable");
        //$("#domining_button").button("disable");

        hide_wallet();
        inline_menu_item_select(document.getElementById('daemon_state_view_menu'));
        disable_tab(document.getElementById('wallet_view_menu'), true);
        //show OK
    }
    else
    {
        //set unknown status
        $("#daemon_status_text").text("Unkonwn state");
        return;
    }

    $("#daemon_status_text").text(info_obj.text_state);

    $("#daemon_out_connections_text").text(info_obj.out_connections_count.toString());
    if(info_obj.out_connections_count >= 8)
    {
        $("#daemon_out_connections_text").addClass("daemon_view_general_status_value_success_text");
    }
    //$("#daemon_inc_connections_text").text(info_obj.inc_connections_count.toString());

    $("#daemon_height_text").text(info_obj.height.toString());
    $("#difficulty_text").text(info_obj.difficulty);
    $("#hashrate_text").text(info_obj.hashrate.toString());
    if(info_obj.last_build_displaymode < 3)
        $("#last_actual_version_text").text("(available version: " + info_obj.last_build_available + ")");
    else
        $("#last_actual_version_text").text("(Critical update: " + info_obj.last_build_available + ")");
    update_last_ver_view(info_obj.last_build_displaymode);
}

function on_update_wallet_status(wal_status)
{

    if(wal_status.wallet_state == 1)
    {
        //syncmode
        $("#transfer_button_id").button("disable");
        $("#synchronizing_wallet_block").show();
        $("#synchronized_wallet_block").hide();
    }else if(wal_status.wallet_state == 2)
    {
        $("#transfer_button_id").button("enable");
        $("#synchronizing_wallet_block").hide();
        $("#synchronized_wallet_block").show();
    }
    else
    {
        alert("wrong state");
    }
}

function build_prefix(count)
{
    var result = "";
    for(var i = 0; i != count; i++)
        result += "0";

    return result;
}

function print_money(amount)
{
    var am_str  = amount.toString();
    if(am_str.length <= 12)
    {
        am_str = build_prefix(13 - am_str.length) + am_str;
    }
    am_str = am_str.slice(0, am_str.length - 12) + "." + am_str.slice(am_str.length - 12);
    return am_str;
}



function get_details_block(td, div_id_str, transaction_id, blob_size)
{
    var res = "<div class='transfer_entry_line_details' id='" + div_id_str + "'> <span class='balance_text'>Transaction ID:</span> " +  transaction_id + ", <b>size</b>: " + blob_size.toString()  + " bytes<br>";
    if(td.rcv !== undefined)
    {
        for(var i=0; i < td.rcv.length; i++)
        {
            res += "<img class='transfer_text_img_span' src='files/income_ico.png' height='15px' /> " + print_money(td.rcv[i]) + "<br>";
        }
    }
    if(td.spn !== undefined)
    {
        for(var i=0; i < td.spn.length; i++)
        {
            res += "<img class='transfer_text_img_span' src='files/outcome_ico.png' height='15px' /> " + print_money(td.spn[i]) + "<br>";
        }
    }

    res += "</div>";
    return res;
}

// format implementation used from http://stackoverflow.com/questions/610406/javascript-equivalent-to-printf-string-format/4673436#4673436
// by Brad Larson, fearphage
if (!String.prototype.format) {
    String.prototype.format = function() {
        var args = arguments;
        return this.replace(/{(\d+)}/g, function(match, number) {
            return typeof args[number] != 'undefined'
                ? args[number]
                : match
                ;
        });
    };
}

function get_transfer_html_entry(tr, is_recent)
{
    var res;
    var color_str;
    var img_ref;
    var action_text;

    if(is_recent)
    {
        color_str = "#6c6c6c";
    }else
    {
        color_str = "#008DD2";
    }

    if(tr.is_income)
    {
        img_ref = "files/income_ico.png"
        action_text = "Received";
    }
    else
    {
        img_ref = "files/outcome_ico.png";
        action_text = "Sent";
    }

    var dt = new Date(tr.timestamp*1000);

    var transfer_line_tamplate = "<div class='transfer_entry_line' style='color: {0}'>";
    transfer_line_tamplate +=       "<img  class='transfer_text_img_span' src='{1}' height='15px'>";
    transfer_line_tamplate +=       "<span class='transfer_text_time_span'>{2}</span>";
    transfer_line_tamplate +=       "<span class='transfer_text_status_span'>{6}</span>";
    transfer_line_tamplate +=       "<span class='transfer_text_details_span'>";
    transfer_line_tamplate +=          "<a href='javascript:;' onclick=\"jQuery('#{4}_id').toggle('fast');\" class=\"options_link_text\"><img src='files/tx_icon.png' height='12px' style='padding-top: 4px'></a>";
    transfer_line_tamplate +=       "</span>";
    transfer_line_tamplate +=       "<span class='transfer_text_amount_span'>{3}</span>";
    transfer_line_tamplate +=       "<span class='transfer_text_recipient_span' title='{7}'>{8}</span>";
    transfer_line_tamplate +=       "{5}";
    transfer_line_tamplate +=     "</div>";

    var short_string = tr.recipient_alias.length ?  "@" + tr.recipient_alias : (tr.recipient.substr(0, 8) + "..." +  tr.recipient.substr(tr.recipient.length - 8, 8) );
    transfer_line_tamplate = transfer_line_tamplate.format(color_str,
        img_ref,
        dt.format("yyyy-mm-dd HH:MM"),
        print_money(tr.amount),
        tr.tx_hash,
        get_details_block(tr.td, tr.tx_hash + "_id", tr.tx_hash, tr.tx_blob_size),
        action_text,
        tr.recipient,
        short_string);

    return transfer_line_tamplate;
}

function on_update_wallet_info(wal_status)
{
    $("#wallet_balance").text(print_money(wal_status.balance));
    $("#wallet_unlocked_balance").text(print_money(wal_status.unlocked_balance));
    $("#wallet_address").text(wal_status.address);
    $("#wallet_path").text(wal_status.path);
}

function on_money_transfer(tei)
{
    $("#recent_transfers_container_id").prepend( get_transfer_html_entry(tei.ti, false));
    $("#wallet_balance").text(print_money(tei.balance));
    $("#wallet_unlocked_balance").text(print_money(tei.unlocked_balance));
}

function on_open_wallet()
{
    Qt_parent.open_wallet();
}

function on_generate_new_wallet()
{
    Qt_parent.generate_wallet();
}

function show_wallet()
{
    //disable_tab(document.getElementById('wallet_view_menu'), false);
    //inline_menu_item_select(document.getElementById('wallet_view_menu'));
    $("#wallet_workspace_area").show();
    $("#wallet_welcome_screen_area").hide();
    $("#recent_transfers_container_id").html("");
}

function hide_wallet()
{
    $("#wallet_workspace_area").hide();
    $("#wallet_welcome_screen_area").show();
}

function on_switch_view(swi)
{
    if(swi.view === 1)
    {
        //switch to dashboard view
        inline_menu_item_select(document.getElementById('daemon_state_view_menu'));
    }else if(swi.view === 2)
    {
        //switch to wallet view
        inline_menu_item_select(document.getElementById('wallet_view_menu'));
    }else
    {
        Qt_parent.message_box("Wrong view at on_switch_view");
    }
}

function on_close_wallet()
{
    Qt_parent.close_wallet();
}


var last_timerId;
function on_transfer()
{
    var transfer_obj = {
        destinations:[
            {
                address: $('#transfer_address_id').val(),
                amount: $('#transfer_amount_id').val()
            }
        ],
        mixin_count: 0,
        payment_id: $('#payment_id').val()
    };

    //if(transfer_obj.destinations[0].address )

    transfer_obj.mixin_count = parseInt($('#mixin_count_id').val());
    transfer_obj.fee = $('#tx_fee').val();

    if(!(transfer_obj.mixin_count >= 0 && transfer_obj.mixin_count < 11))
    {
        Qt_parent.message_box("Wrong Mixin parameter value, please set values in range 0-10");
        return;
    }

    var transfer_res_str = Qt_parent.transfer(JSON.stringify(transfer_obj));
    var transfer_res_obj = jQuery.parseJSON(transfer_res_str);

    if(transfer_res_obj.success)
    {


        $('#transfer_address_id').val("");
        $('#transfer_amount_id').val("");
        $('#payment_id').val("");
        $('#mixin_count_id').val(0);
        $("#transfer_result_span").html("<span style='color: #1b9700'> Money successfully sent, transaction " + transfer_res_obj.tx_hash + ", " + transfer_res_obj.tx_blob_size + " bytes</span><br>");
        if(last_timerId !== undefined)
            clearTimeout(last_timerId);

        $("#transfer_result_zone").show("fast");
        last_timerId = setTimeout(function(){$("#transfer_result_zone").hide("fast");}, 15000);
    }
    else
        return;
}

function on_set_recent_transfers(o)
{
    if(o === undefined || o.history === undefined || o.history.length === undefined)
        return;

    var str = "";
    for(var i=0; i < o.history.length; i++)
    {
        str += get_transfer_html_entry(o.history[i], true);
    }
    $("#recent_transfers_container_id").prepend(str);
}


function str_to_obj(str)
{
    var info_obj = jQuery.parseJSON(str);
    this.cb(info_obj);
}

$(function()
{ // DOM ready
    $( "#synchronization_progressbar" ).progressbar({value: false });
    $( "#wallet_progressbar" ).progressbar({value: false });
    $(".common_button").button();

    $("#open_wallet_button").button("disable");
    $("#generate_wallet_button").button("disable");
    $("#domining_button").button("disable");


    //$("#transfer_button_id").button("disable");
    $('#open_wallet_button').on('click',  on_open_wallet);
    $('#transfer_button_id').on('click',  on_transfer);
    $('#generate_wallet_button').on('click',  on_generate_new_wallet);
    $('#close_wallet_button_id').on('click',  on_close_wallet);

    /****************************************************************************/
    //some testing stuff
    //to make it available in browser mode
    show_wallet();
    on_update_wallet_status({wallet_state: 2});
    var tttt = {
        ti:{
            height: 10,
            tx_hash: "b19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c212fe",
            amount: 10111100000000,
            tx_blob_size: 1222,
            is_income: true,
            timestamp: 1402878665,
            td: {
                rcv: [1000, 1000, 1000, 1000],//rcv: ["0.0000001000", "0.0000001000", "0.0000001000", "0.0000001000"],
                spn: [1000, 1000]//spn: ["0.0000001000", "0.0000001000"]
            },
            recipient: "1Htb4dS5vfR53S5RhQuHyz7hHaiKJGU3qfdG2fvz1pCRVf3jTJ12mia8SJsvCo1RSRZbHRC1rwNvJjkURreY7xAVUDtaumz",
            recipient_alias: "just-mike"
        },
        balance: 1000,
        unlocked_balance: 1000
    };

    $("#recent_transfers_container_id").prepend( get_transfer_html_entry(tttt.ti, true));
    $("#recent_transfers_container_id").prepend( get_transfer_html_entry(tttt.ti, true));

    on_money_transfer(tttt);
    tttt.ti.tx_hash = "b19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = false;
    tttt.ti.timestamp = 1402171355;
    tttt.ti.amount =  10123000000000;

    on_money_transfer(tttt);

    tttt.ti.tx_hash = "u19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = false;
    on_money_transfer(tttt);

    tttt.ti.tx_hash = "s19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = true;
    on_money_transfer(tttt);

    tttt.ti.tx_hash = "q19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = false;
    on_money_transfer(tttt);
    /****************************************************************************/



    inline_menu_item_select(document.getElementById('daemon_state_view_menu'));

    //inline_menu_item_select(document.getElementById('wallet_view_menu'));

    Qt.update_daemon_state.connect(str_to_obj.bind({cb: on_update_daemon_state}));
    Qt.update_wallet_status.connect(str_to_obj.bind({cb: on_update_wallet_status}));
    Qt.update_wallet_info.connect(str_to_obj.bind({cb: on_update_wallet_info}));
    Qt.money_transfer.connect(str_to_obj.bind({cb: on_money_transfer}));
    Qt.show_wallet.connect(show_wallet);
    Qt.hide_wallet.connect(hide_wallet);
    Qt.switch_view.connect(on_switch_view);
    Qt.set_recent_transfers.connect(str_to_obj.bind({cb: on_set_recent_transfers}));


    hide_wallet();
    on_update_wallet_status({wallet_state: 1});
    // put it here to disable wallet tab only in qt-mode
    disable_tab(document.getElementById('wallet_view_menu'), true);
    $("#version_text").text(Qt_parent.get_version());
});
