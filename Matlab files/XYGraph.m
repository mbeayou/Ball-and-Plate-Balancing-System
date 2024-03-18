

% Plot the first column of xgraph and ygraph
figure;
plot(t, xgraph(:, 1), 'r-', 'LineWidth', 2, 'DisplayName', 'xgraph');
hold on;
plot(t, ygraph(:, 1), 'b-', 'LineWidth', 2, 'DisplayName', 'ygraph');
xlabel('Time');
ylabel('Amplitude');
title('XY Graph');
legend('show');
grid on;

% Set y-axis limits
ylim([300, 800]);

hold off;