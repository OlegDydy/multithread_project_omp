function Pair(_key,_value){
	return {key:_key,value:_value};
}

function hypot(x,y){
	x = Math.abs(x);
	y = Math.abs(y);
	var t = Math.min(x,y);
	x = Math.max(x,y);
	if (x == 0)
			return 0;
	t = t/x;
	return x * Math.sqrt(1+t*t);
}

function Arrow(context,x1,y1,x2,y2){
	var ang = Math.PI / 12.0;
	var M = [Math.cos(ang),Math.sin(ang),-Math.sin(ang),Math.cos(ang)];
	var norm = 20.0 / hypot(x2 - x1, y2 - y1);
	var vx1 = (x1 - x2) * norm;
	var vy1 = (y1 - y2) * norm;
	
	context.beginPath();
	context.moveTo(x1,y1);
	context.lineTo(x2,y2);
	context.lineTo(x2 + M[0] * vx1 + M[1] * vy1, y2 + M[2] * vx1 + M[3] * vy1);

	context.moveTo(x2,y2);
	context.lineTo(x2 + M[0] * vx1 + M[2] * vy1, y2 + M[1] * vx1 + M[3] * vy1);
	context.stroke();
}

function log_n(value,base){
	return Math.log(value)/Math.log(base);
}
function log10(value,base){
	const anti = 1.0/Math.LN10;
	return Math.log(value) * anti;
}
var context = undefined;

function Circle(context,x,y,r){
	context.beginPath();
	context.arc(x,y,r,0,2 * Math.PI,true);
	context.stroke();
}

function plot2d(drawingCanvas, data) {
	if (typeof(data) != "object")
		return;
	
	multiplot = !!data[0][0];
	
	if(drawingCanvas && drawingCanvas.getContext) {
	context = drawingCanvas.getContext('2d');
	const height = drawingCanvas.height;
	const width = drawingCanvas.width;
	const graph_y = 0.9;
	const ws_y = 1.0 - graph_y;
	const shift_x = 40;
	const shift_y = 0.5 * ws_y * height;
	
	var m_x;
	var m_y;
	if (multiplot){
		var m_x = data[0][0].key;
		var m_y = data[0][0].value;
	}else{
		m_x = data[0].key;
		m_y = data[0].value;
	}
	var n_x = 0.0;
	var n_y = 0.0;
	
	if (multiplot){
		for (var j = 0;j < data.length;j++)
			for (var i = 0;i < data[j].length;i++){
				m_y = Math.max(m_y,data[j][i].value);
				m_x = Math.max(m_x,data[j][i].key);
				//n_y = Math.min(n_y,data[j][i].value);
				//n_x = Math.min(n_x,data[j][i].key);
			}
	}
	else
		for (var i = 0;i < data.length;i++){
			m_y = Math.max(m_y,data[i].value);
			m_x = Math.max(m_x,data[i].key);
			//n_y = Math.min(n_y,data[i].value);
			//n_x = Math.min(n_x,data[i].key);
		}
	var scale = log10(m_x - n_x);
	var n = Math.trunc(scale);
	scale = (width - shift_x) / scale
	var norm = height * graph_y / m_y;
	// оси
	{
		const serif_h = 5;
		Arrow(context, shift_x, height - shift_y, width, height - shift_y);
		Arrow(context, shift_x, height - shift_y, shift_x, shift_y);
		context.beginPath();
		
		// Засечки по x
		context.textBaseline = "top";
		context.textAlign = "center";
		// ноль
		context.fillText("0", shift_x, height - shift_y + serif_h);
		for (var i = 1;i <= n;i++){
			x = shift_x + i * scale;
			context.moveTo(x,height - shift_y - serif_h);
			context.lineTo(x,height - shift_y + serif_h);
			// подписи
			if (i > 5){
				context.fillText("1.0e+" + i , x, height - shift_y + serif_h);
			}else{
				context.fillText(Math.trunc(Math.exp(i * Math.LN10)),x,height - shift_y + serif_h);
			}			
		}
		// Засечки по y
		context.textBaseline = "middle";
		context.textAlign = "right";
		var e = Math.trunc(Math.log(m_y - n_y) / Math.LN10 );
		n = Math.abs(Math.round( m_y / Math.exp(e * Math.LN10) ) - 5);
		var step = 1.0;
		var dy = m_y - n_y;
		if ( m_y != n_y)
			dy = m_y - n_y;
		else{
			if (m_y != 0)
				dy = m_y;
			else
				dy = 1.0;
		}
		
		if (dy < 1){
			for (var j = 1; j > dy; j /= 10.0);
			step = j;
		}else{
			for (var j = 1; j < dy; j *= 10.0);
			step = j / 10.0;
		}
		if (dy / step < 5)
			step *= 0.2; 
		
		for (var y = step;y <= m_y;y += step){
			ry = height - shift_y - y * norm;
			context.moveTo(shift_x - serif_h,ry);
			context.lineTo(shift_x + serif_h,ry);
			// подписи
			context.fillText(y.toPrecision(5), shift_x - serif_h, ry);			
		}
	
		context.stroke();
		// подписи осей
		// OX
		context.font = "16px sans-serif";
		context.textBaseline = "bottom";
		context.textAlign = "right";
		context.fillText("log10(N)", width, height - shift_y - 10);
		// OY
		context.font = "16px sans-serif";
		context.textBaseline = "top";
		context.textAlign = "left";
		context.fillText("t, ms.", shift_x + 10, shift_y);
	}
	// вывод графика
	const r = 3;
	if (multiplot){
		const colors = ["blue","green","red","magenta","orange","cyan"]
		for (var j = 0;j < data.length;j++)
		{
			context.strokeStyle = colors[j];
			context.beginPath();
			context.moveTo(shift_x + log10(data[j][0].key) * scale,height * graph_y + shift_y - data[j][0].value * norm);

			for (var i = 1;i<data[j].length;i++)
				context.lineTo(shift_x + log10(data[j][i].key) * scale,height * graph_y + shift_y - data[j][i].value * norm);
			context.stroke();			

			//for (var i = 0;i<data[j].length;i++)
			//	Circle(context,shift_x + log10(data[j][i].key) * scale, height * graph_y + shift_y - data[j][i].value * norm,r);
		}
	}else{	
		context.strokeStyle = "blue";
		context.beginPath();
		context.moveTo(log10(data[0].key) * scale,height * graph_y + shift_y - data[0].value * norm);

		for (var i = 0;i<data.length;i++)
			context.lineTo(log10(data[i].key) * scale,height * graph_y + shift_y - data[i].value * norm);
		context.stroke();

		//for (var i = 0;i<data.length;i++)
		//	Circle(context, log10(data[i].key) * scale, height * graph_y + shift_y - data[i].value * norm,r);
	}
	}
}