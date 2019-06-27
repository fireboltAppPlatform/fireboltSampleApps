
px.import({scene : 'px:scene.1.js',
           keys: 'px:tools.keys.js'
}).then( function importsAreReady(imports) {

var scene = imports.scene;
var keys = imports.keys;
var controls = null;
var player = null;
var externalObj = null;

var IP_VOD=0;
var IP_CDVR=1;
var IP_LINEAR=2;
var QAM_LINEAR=3;//qam test will only be enabled on cable and hybrid STBs

var streams = [
    //IP VOD
    [
        "https://mnmedias.api.telequebec.tv/m3u8/29880.m3u8",
    ],
    //IP CDVR
    [
        "https://mnmedias.api.telequebec.tv/m3u8/29880.m3u8",
    ],
    //IP LINEAR
    [
        "https://mnmedias.api.telequebec.tv/m3u8/29880.m3u8",
    ]
]
var stream_id = [0,0,0,0];
var stream_type = IP_LINEAR;//IP_VOD,IP_CDVR,IP_LINEAR,QAM_LINEAR

externalObj = scene.create( {t:"external", x:0, y:0, w:1280, h:720, parent:scene.root,cmd:"rdkmediaplayer"} );
externalObj.remoteReady.then(handleExternalRemoteSuccess, handleExternalRemoteError);


function handleExternalRemoteSuccess(external)
{
    console.log("Handle external success");
    externalObj = external;
    externalObj.moveToBack();
    player = external.api;
    registerPlayerEvents();
    loadStream();
}

function handleExternalRemoteError(error)
{
    console.log("Handle external error");
}

function registerPlayerEvents()
{
    console.log("registerPlayerEvents");
    player.on("onMediaOpened",onMediaOpened);
    player.on("onStatus",onEvent);
    player.on("onWarning",onEvent);
}

function loadStream()
{
    console.log("loadStream")
    doLoad(getCurrentStreamURL());
}

function getCurrentStreamURL()
{
    return streams[stream_type][stream_id[stream_type]];
}

function doLoad(url)
{
    console.log("doLoad:" + url);
    player.url = url;
    player.setVideoRectangle(0,0,scene.w,scene.h);
    player.play();
}

function onMediaOpened(e)
{
    console.log("Event " + e.name
            + "\n   type:" + e.mediaType
            + "\n   width:" + e.width
            + "\n   height:" + e.height
            + "\n   speed:" + e.availableSpeeds
            + "\n   sap:" + e.availableAudioLanguages
            + "\n   cc:" + e.availableClosedCaptionsLanguages
    );
}

function onEvent(e)
{
    console.log("Event " + e.name);
}

}).catch( function importFailed(err){
    console.error("Import failed for rdkmediaplayer.js: " + err)
});
