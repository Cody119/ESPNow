%%SimplePlot
Colors
data=csvread('test1_rssi_data.csv'); % reading csv file 

[~, Y] = size(data);
figure;
for i = 2:(Y/2 - 1)
    r1 = data(:,i*2-1); % reading its first column
    r2 = data(:,i*2); % reading its second column of csv file
    x = r1(2:end, :) ./ 1000;
    y = r2(2:end, :);
    zeroed = x ~= 0;
    hold on;
    scatter(x(zeroed),y(zeroed), 10, colors{i}, 'filled');
end

set(gca,'Color','k');
set(gcf,'color','k');
set(gca,'ycolor','w');
set(gca,'xcolor','w');

labels = {};
for i = 2:(floor(Y/2)-1)
    labels{i-1} = "\color{white} " + num2str(data(1, i*2-1)) + "m to " + num2str(data(1, i*2-1) + 2.5) + "m";
end

legend(labels)
    
%grid on
xlabel('\color{white} Time (s)', 'fontsize', 15);
ylabel('\color{white} RSSI', 'fontsize', 15);
% legend('');