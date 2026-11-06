param([string]$Layout='build/android-032-nine-layout.json',[string]$Output='build/android-040-nine-preview.png',[switch]$DemoNineInput)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$model=Get-Content -Encoding UTF8 -Raw -LiteralPath $Layout | ConvertFrom-Json
$scale=3
$extra=0
if($DemoNineInput){$extra=68}
$bitmap=[Drawing.Bitmap]::new([int]($model.width*$scale),[int](($model.height+42+$extra)*$scale))
$g=[Drawing.Graphics]::FromImage($bitmap)
$g.ScaleTransform($scale,$scale)
$g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint=[Drawing.Text.TextRenderingHint]::AntiAliasGridFit
function Brush($hex){[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml($hex))}
function Round($x,$y,$w,$h,$color){
 $path=[Drawing.Drawing2D.GraphicsPath]::new();$radius=10
 $path.AddArc([single]$x,[single]$y,$radius,$radius,180,90);$path.AddArc([single]($x+$w-$radius),[single]$y,$radius,$radius,270,90)
 $path.AddArc([single]($x+$w-$radius),[single]($y+$h-$radius),$radius,$radius,0,90);$path.AddArc([single]$x,[single]($y+$h-$radius),$radius,$radius,90,90);$path.CloseFigure()
 $b=Brush $color;$g.FillPath($b,$path);$b.Dispose();$path.Dispose()
}
function Caption($text,$x,$y,$w,$h,$size,$color){
 $font=[Drawing.Font]::new('Microsoft YaHei UI',[single]$size,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
 $format=[Drawing.StringFormat]::new();$format.Alignment=[Drawing.StringAlignment]::Center;$format.LineAlignment=[Drawing.StringAlignment]::Center;$format.FormatFlags=[Drawing.StringFormatFlags]::NoWrap
 $measured=$g.MeasureString($text,$font).Width
 if($measured -gt ($w-6)){ $font.Dispose();$font=[Drawing.Font]::new('Microsoft YaHei UI',[single]($size*($w-6)/$measured),[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel) }
 $b=Brush $color;$g.DrawString($text,$font,$b,[Drawing.RectangleF]::new($x,$y,$w,$h),$format);$b.Dispose();$font.Dispose();$format.Dispose()
}
$g.Clear([Drawing.ColorTranslator]::FromHtml('#DCE3DC'))
Caption '键盘' ($model.width-173) 0 45 42 14 '#24282F'
Caption '设置' ($model.width-128) 0 45 42 14 '#24282F'
Caption '切换' ($model.width-83) 0 43 42 14 '#24282F'
Caption '⌄' ($model.width-40) 0 36 42 20 '#24282F'
if($DemoNineInput){
 # Illustration of the 7664 core regression fixture, not a device capture.
 # Move the toolbar down below preedit, matching the service's view order.
 $g.Clear([Drawing.ColorTranslator]::FromHtml('#DCE3DC'))
 Caption 'rong' 4 0 80 24 13 '#45515C'
 Caption '键盘' ($model.width-173) 24 45 42 14 '#24282F'
 Caption '设置' ($model.width-128) 24 45 42 14 '#24282F'
 Caption '切换' ($model.width-83) 24 43 42 14 '#24282F'
 Caption '⌄' ($model.width-40) 24 36 42 20 '#24282F'
 $words=@('容','送','松','宋');for($i=0;$i -lt 4;$i++){Caption $words[$i] ($i*50) 66 50 44 19 $(if($i -eq 0){'#247983'}else{'#24282F'})}
 Caption '‹' ($model.width-106) 66 32 44 18 '#858D98';Caption '›' ($model.width-74) 66 32 44 18 '#24282F';Caption '展开' ($model.width-42) 66 42 44 14 '#24282F'
}
foreach($key in $model.keys){
 if($DemoNineInput -and $key.style -eq 4){continue}
 $x=[single]$key.x;$y=[single]($key.y+42+$extra);$w=[single]$key.width;$h=[single]$key.height
 Round ($x+.5) ($y+1.5) ($w-1) ($h-1.5) '#CBD5CB'
 Round ($x+.5) ($y+.5) ($w-1) ($h-1.5) $(if($key.style -eq 1){'#E4ECE5'}else{'#FCFCFD'})
 if($key.action -eq '⌫' -or $key.action -eq '↵'){
  $pen=[Drawing.Pen]::new([Drawing.ColorTranslator]::FromHtml('#24282F'),1.6);$cx=$x+$w/2;$cy=$y+$h/2;$s=11
  if($key.action -eq '⌫'){
   $pts=[Drawing.PointF[]]@([Drawing.PointF]::new($cx-$s,$cy),[Drawing.PointF]::new($cx-$s*.35,$cy-$s*.65),[Drawing.PointF]::new($cx+$s,$cy-$s*.65),[Drawing.PointF]::new($cx+$s,$cy+$s*.65),[Drawing.PointF]::new($cx-$s*.35,$cy+$s*.65))
   $g.DrawPolygon($pen,$pts);$g.DrawLine($pen,[single]($cx-1),[single]($cy-3),[single]($cx+5),[single]($cy+3));$g.DrawLine($pen,[single]($cx-1),[single]($cy+3),[single]($cx+5),[single]($cy-3))
  }else{$g.DrawLine($pen,[single]($cx+$s),[single]($cy-6),[single]($cx+$s),[single]($cy+3));$g.DrawLine($pen,[single]($cx+$s),[single]($cy+3),[single]($cx-$s),[single]($cy+3));$g.DrawLine($pen,[single]($cx-$s),[single]($cy+3),[single]($cx-5),[single]($cy-3));$g.DrawLine($pen,[single]($cx-$s),[single]($cy+3),[single]($cx-5),[single]($cy+9))}
  $pen.Dispose();continue
 }
 if($key.hint){Caption $key.hint $x ($y+$h*.04) $w ($h*.30) 12 '#858D98';Caption $key.label $x ($y+$h*.31) $w ($h*.60) $(if($key.label -eq '分词'){[Math]::Min(19,$h*.43)}else{[Math]::Min(26,$h*.43)}) '#24282F'}
 else{Caption $key.label $x $y $w $h $(if($key.style -eq 4){[Math]::Min(29,$h*.80)}elseif($key.label.Length -gt 3){17}else{22}) '#24282F'}
}
if($DemoNineInput){
 Round 4 (46+$extra) 48 157 '#EDF4EE'
 Caption 'rong' 4 (46+$extra) 48 40 14 '#24282F'
 Caption 'song' 4 (86+$extra) 48 40 14 '#24282F'
}
$bitmap.Save((Join-Path $PWD $Output),[Drawing.Imaging.ImageFormat]::Png)
$g.Dispose();$bitmap.Dispose()
Write-Output "Layout illustration saved: $Output (uses actual key geometry; not a device screenshot)"
