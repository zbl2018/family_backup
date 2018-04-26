<?php
session_start();

if(isset($_SESSION['user'])=="")
{
 header("Location: index.php");
}
$play_path="user_info/user_".$_SESSION['user']."/video/".$_GET["play_id"];
//echo "user_info/user_".$_SESSION['user']."/video/".$_GET["play_id"];
//sleep(3);
$con = new mysqli('localhost','root', '4817','web');
if (!$con)
 {
 die('Could not connect: ' . mysql_error());
 }
 $con->set_charset("utf8");//或者 $mysqli->query("set names 'utf8'")
//mysql_select_db("ajax_demo", $con);
$sql="SELECT video_name FROM video_info WHERE user_id = '".$_SESSION['user']."'";
// echo "111";
//echo $row['air_condition_id'];
$res = $con->query($sql);
?>

<!DOCTYPE html>
<html lang="utf-8">
<meta name="viewport" charset="utf-8" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<head>
</head>
<title>视频显示</title>
<body style="text-align:center;">
<p>
	<video width="300" height="300" controls="controls"  autoplay="autoplay" loop="900"> 
	<source src=<?php echo($play_path)?> type="video/mp4"> 
		您的浏览器不支持此种视频格式。 
</video>
	</p>
点击 	
<a href="logout.php">退出登录？</a>
或者点击 
<a href="client.php">返回控制台</a>
<p>
<p align="left">
以下是你家里最近的视频缓存，点击以下某个具体时间查看监控录像：
</p>
 <?php   
while($row=$res->fetch_array())
 {
    echo "<p><a href= video.php?play_id=".$row["video_name"].">".$row["video_name"]."</a></p>";
 }

$con->close();
?>
</p>
</body>  

</html>