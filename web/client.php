<?php
session_start();
if(isset($_SESSION['user'])=="")
{
 header("Location: index.php");
}
$user_id = $_SESSION['user'];
?>
<html>
<head>
<title>WebSocket在线控制平台</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<style>
html,body{font:normal 0.9em arial,helvetica;text-align:center}
#log {margin:0 auto;width:440px; height:200px; border:1px solid #7F9DB9; overflow:auto;}
#msg {margin:0 auto;width:330px;}
</style>
<script>
var socket;
function init(){
	if(!("WebSocket" in window)){ log('当前浏览器不支持WebSocket');return; }
	var host = "ws://193.112.128.66:20000";
	try{
		socket = new WebSocket(host);
		log('WebSocket初始化状态 '+socket.readyState);
		socket.onopen = function(msg){ 
			log("欢迎进入控制系统 - 状态 "+this.readyState);
			var link_res = '{"action":"link_txt","user_id":<?php echo $user_id?>}';
			this.send(link_res);
			};
		socket.onmessage = function(msg){			 
			//log("<返回>信息: "+msg.data);
		 };
		socket.onclose = function(msg){ log("连接关闭 - 状态 "+this.readyState); };
	}
	catch(ex){ log(ex); }
	$("msg").focus();
}
function send(){
	// var txt,msg;
	// txt = $("msg");
	// msg = txt.value;
	// //if(!msg){ alert("Message can not be empty"); return; }
	// txt.value="";
	// txt.focus();
	var jsonString = '{"action":"move","angle":"0","mode":1,"speed":"0.1"}';
	// var jsObject = JSON.parse(jsonString); //转换为json对象

	// alert(jsObject.bar); //取json中的值

	// var st = JSON.stringify(jsObject); //转换为json类型的字符
	// while(true){
		try{ socket.send(jsonString); log('您发送了: '+jsonString); } catch(ex){ log(ex); }
	// 	<?php
	// 	   usleep(1000000);
	// 	?>
	// }
	
}

function send2(){
	var jsonString = '{"action":"move","angle":"-90","mode":1,"speed":"0.1"}';
	try{ socket.send(jsonString); log('您发送了: '+jsonString); } catch(ex){ log(ex); }
}
function quit(){
	log("Goodbye!");
	socket.close();
	socket=null;
}
function $(id){ return document.getElementById(id); }
function log(msg){ $("log").innerHTML+="<br />"+msg; $("log").scrollTop=$("log").scrollHeight;}
function onkey(event){ if(event.keyCode==13){ send(); } }
</script>
</head>
<body style="text-align:center;" onLoad="init()">
	<h3>在线控制系统</h3>
	<div id="log"></div>
	<!-- <input id="msg" type="textbox" onKeyPress="onkey(event)"/> -->
	<button onClick="send()">开灯</button>
	<button onClick="send2()">关灯</button>
	<button onClick="quit()">断开控制台链接</button>
	<button onClick="init()">重新连接控制台</button>
	<p>
	<a href="video.php"><button>查看往期的摄像视频</button></input></a>
	<a href="logout.php"><button>退出登录</button></input></a>
	</p>
	<p>
	<img src="" height=300 width=300 id="imgDiv"/>
	</p>	
    <script type="text/javascript" charset="utf-8"> 
    var ws = new WebSocket("ws://193.112.128.66:20000");
	//$user_id = $_SESSION['user'];
	// alert($user_id);
    var img = document.getElementById("imgDiv");
	ws.onopen = function(msg){ 	
			var link_res = '{"action":"link_photo","user_id":<?php echo $user_id?>}';
			this.send(link_res); 
			};
    ws.onmessage = function(evt){
    if(typeof(evt.data)=="string"){
        //textHandler(JSON.parse(evt.data));  
    }else{  
    //alert(evt.data)
  	var reader = new FileReader();  
        reader.onload = function(evt){  
            if(evt.target.readyState == FileReader.DONE){  
                var url = evt.target.result;  
                //alert(url);  
                img.src = url;  
            }  
        }  
        reader.readAsDataURL(evt.data);  
    }  
}
</script>
	<!-- <div>命令: id：查看会话id,date：查看当前日期,time：查看当前时间,exit：慎用，结束服务器端运行</div> -->
</body>
</html>
