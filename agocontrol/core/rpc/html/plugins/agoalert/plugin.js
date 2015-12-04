/**
 * Agoalert plugin
 * @returns {agoalert}
 */
function agoAlertPlugin(deviceMap) {
    //members
    var self = this;
    self.hasNavigation = ko.observable(false);
    self.gtalkStatus = ko.observable(self.gtalkStatus);
    self.gtalkUsername = ko.observable(self.gtalkUsername);
    self.gtalkPassword = ko.observable(self.gtalkPassword);
    self.smsStatus = ko.observable(self.smsStatus);
    self.twelvevoipUsername = ko.observable(self.twelvevoipUsername);
    self.twelvevoipPassword = ko.observable(self.twelvevoipPassword);
    self.mailStatus = ko.observable(self.mailStatus);
    self.mailSmtp = ko.observable(self.mailSmtp);
    self.mailLogin = ko.observable(self.mailLogin);
    self.mailPassword = ko.observable(self.mailPassword);
    self.mailTls = ko.observable(self.mailTls);
    self.mailSender = ko.observable(self.mailSender);
    self.twitterStatus = ko.observable(self.twitterStatus);
    self.pushStatus = ko.observable(self.pushStatus);
    self.selectedPushProvider = ko.observable(self.selectedPushProvider);
    self.pushbulletSelectedDevices = ko.observableArray();
    self.pushbulletAvailableDevices = ko.observableArray();
    self.pushbulletApikey = ko.observable();
    self.pushoverUserid = ko.observable();
    self.nmaApikey = ko.observable(self.nmaApikey);
    self.nmaAvailableApikeys = ko.observableArray();
    self.nmaSelectedApikeys = ko.observableArray();
    self.agoalertUuid;
    
    //get agoalert uuid
    if( deviceMap!==undefined )
    {
        for( var i=0; i<deviceMap.length; i++ )
        {
            if( deviceMap[i].devicetype=='alertcontroller' )
            {
                self.agoalertUuid = deviceMap[i].uuid;
            }
        }
    }

    //get current status
    self.getAlertsConfigs = function()
    {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'status';
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                self.gtalkStatus(true);
                if (res.result.gtalk.configured)
                {
                    self.gtalkUsername(res.result.gtalk.username);
                    self.gtalkPassword(res.result.gtalk.password);
                }
                self.mailStatus(res.result.mail.configured);
                if (res.result.mail.configured)
                {
                    self.mailSmtp(res.result.mail.smtp);
                    self.mailLogin(res.result.mail.login);
                    self.mailPassword(res.result.mail.password);
                    if (res.result.mail.tls == 1)
                        self.mailTls(true);
                    else
                        self.mailTls(false);
                    self.mailSender(res.result.mail.sender);
                }
                self.smsStatus(res.result.sms.configured);
                if (res.result.sms.configured)
                {
                    self.twelvevoipUsername(res.result.sms.username);
                    self.twelvevoipPassword(res.result.sms.password);
                }
                self.twitterStatus(res.result.twitter.configured);
                self.pushStatus(res.result.push.configured);
                if (res.result.push.configured)
                {
                    self.selectedPushProvider(res.result.push.provider);
                    if (res.result.push.provider == 'pushbullet')
                    {
                        self.pushbulletApikey(res.result.push.apikey);
                        self.pushbulletAvailableDevices(res.result.push.devices);
                        self.pushbulletSelectedDevices(res.result.push.devices);
                    }
                    else if (res.result.push.provider == 'pushover')
                    {
                        self.pushoverUserid(res.result.push.userid);
                    }
                    else if (res.result.push.provider == 'notifymyandroid')
                    {
                        self.nmaAvailableApikeys(res.result.push.apikeys);
                    }
                }
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Unable to get alert modules status');
            }
        });
    };

    this.twitterUrl = function()
    {
        var el = $('#twitterUrl');
        if( el===undefined )
        {
            notif.error('#ie');
            return;
        }
        var generatingUrl = $('#gu') || 'generating url';
        var authorizationUrl = $('#au') || 'authorization url';

        //get authorization url
        el.html(generatingUrl.html()+'...');
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'setconfig';
        content.param1 = 'twitter';
        content.param2 = '';
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            { 
                if (res.result.error == 0)
                {
                    //display link
                    el.html('<a href="' + res.result.url + '" target="_blank">'+authorizationUrl.html()+'</a>');
                }
                else
                {
                    notif.error('Unable to get Twitter url');
                }
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to get twitter url');
            }
        });
    };

    this.twitterAccessCode = function()
    {
        var el = $('#twitterAccessCode');
        if( el===undefined )
        {
            notif.error('#ie');
            return;
        }
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'setconfig';
        content.param1 = 'twitter';
        content.param2 = el.val();
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                if (res.result.error == 1)
                {
                    notif.error(res.result.msg);
                }
                self.getAlertsConfigs();
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to get twitter access code');
            }
        });
    };

    this.twitterTest = function()
    {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'test';
        content.param1 = 'twitter';
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                notif.info(res.result.msg);
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to send twitter test');
            }
        });
    };

    this.smsConfig = function() {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'setconfig';
        content.param1 = 'sms';
        content.param2 = self.twelvevoipUsername();
        content.param3 = self.twelvevoipPassword();
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                if (res.result.error == 1)
                {
                    notif.error(res.result.msg);
                }
                self.getAlertsConfigs();
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to save sms config');
            }
        });
    };

    this.smsTest = function()
    {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'test';
        content.param1 = 'sms';
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                notif.info(res.result.msg);
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to send sms test');
            }
        });
    };

    this.gtalkConfig = function()
    {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'setconfig';
        content.param1 = 'gtalk';
        content.param2 = self.gtalkUsername();
        content.param3 = self.gtalkPassword();
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                if (res.result.error == 1)
                {
                    notif.error(res.result.msg);
                }
                self.getAlertsConfigs();
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to save gtalk config');
            }
        });
    };

    this.gtalkTest = function()
    {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'test';
        content.param1 = 'gtalk';
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                notif.info(res.result.msg);
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to send gtalk test');
            }
        });
    };

    this.mailConfig = function()
    {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'setconfig';
        content.param1 = 'mail';
        content.param2 = self.mailSmtp();
        content.param3 = self.mailSender();
        content.param4 = '';
        if( self.mailLogin()!==undefined )
        {
            content.param4 += self.mailLogin();
        }
        content.param4 += '%_%';
        if( self.mailPassword()!==undefined )
        {
            content.param4 += self.mailPassword();
        }
        content.param5 = '0';
        if (self.mailTls())
        {
            content.param5 = '1';
        }
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                if (res.result.error == 1)
                {
                    notif.error(res.result.msg);
                }
                self.getAlertsConfigs();
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to save email config');
            }
        });
    };

    this.mailTest = function()
    {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'test';
        content.param1 = 'mail';
        content.param2 = document.getElementsByClassName("mailEmail")[0].value;
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                notif.info(res.result.msg);
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to send email test');
            }
        });
    };

    this.pushbulletRefreshDevices = function()
    {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'setconfig';
        content.param1 = 'push';
        content.param2 = this.selectedPushProvider();
        content.param3 = 'getdevices';
        content.param4 = self.pushbulletApikey();
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                if (res.result.error == 0)
                {
                    notif.success('#rdpbs');
                    self.pushbulletAvailableDevices(res.result.devices);
                }
                else
                {
                    notif.error('#rdpb');
                }
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to get push devices');
            }
        });
    };

    this.nmaAddApikey = function()
    {
        if (self.nmaApikey().length > 0)
        {
            self.nmaAvailableApikeys.push(self.nmaApikey());
        }
        self.nmaApikey('');
    };

    this.nmaDelApikey = function()
    {
        for ( var j = self.nmaSelectedApikeys().length - 1; j >= 0; j--)
        {
            for ( var i = self.nmaAvailableApikeys().length - 1; i >= 0; i--)
            {
                if (self.nmaAvailableApikeys()[i] === self.nmaSelectedApikeys()[j])
                {
                    self.nmaAvailableApikeys().splice(i, 1);
                }
            }
        }
        self.nmaAvailableApikeys(self.nmaAvailableApikeys());
    };

    this.pushConfig = function()
    {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'setconfig';
        content.param1 = 'push';
        content.param2 = this.selectedPushProvider();
        if (this.selectedPushProvider() == 'pushbullet')
        {
            content.param3 = 'save';
            content.param4 = this.pushbulletApikey();
            content.param5 = this.pushbulletSelectedDevices();
        }
        else if (this.selectedPushProvider() == 'pushover')
        {
            content.param3 = this.pushoverUserid();
        }
        else if (this.selectedPushProvider() == 'notifymyandroid')
        {
            content.param3 = this.nmaAvailableApikeys();
        }
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                if (res.result.error == 1)
                {
                    notif.error(res.result.msg);
                }
                self.getAlertsConfigs();
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to save push config');
            }
        });
    };

    this.pushTest = function()
    {
        var content = {};
        content.uuid = self.agoalertUuid;
        content.command = 'test';
        content.param1 = 'push';
        sendCommand(content, function(res) {
            if( res!==undefined && res.result!==undefined && res.result!=='no-reply')
            {
                notif.info(res.result.msg);
            }
            else
            {
                 notif.fatal('#nr');
                 console.log('Agoalert is not responding: Unable to send twitter test');
            }
        });
    };
}

/**
 * Entry point: mandatory!
 */
function init_plugin()
{
    ko.bindingHandlers.jqTabs = {
        init: function(element, valueAccessor) {
            //init
            var options = valueAccessor() || {};
            setTimeout( function() { $(element).tabs(options); }, 0);
            //load config
            model.getAlertsConfigs();
        }
    };

    model = new agoAlertPlugin(deviceMap);
    model.mainTemplate = function() {
	    return templatePath + "agoalert";
    }.bind(model);
    ko.applyBindings(model);
}
